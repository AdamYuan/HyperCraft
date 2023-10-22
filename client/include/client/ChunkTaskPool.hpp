#pragma once
#ifndef HC_CLIENT_CHUNK_TASK_POOL_HPP
#define HC_CLIENT_CHUNK_TASK_POOL_HPP

#include <atomic>
#include <concurrentqueue.h>
#include <condition_variable>
#include <cuckoohash_map.hh>
#include <glm/gtx/hash.hpp>
#include <memory>
#include <mutex>
#include <optional>
#include <tuple>
#include <variant>

namespace hc::client {

class World;
class Chunk;
class ChunkTaskPool;
class ChunkTaskPoolLocked;

enum class ChunkTaskType { kGenerate, kMesh, kSetBlock, kLight, COUNT };
enum class ChunkTaskPriority { kHigh, kLow };

template <ChunkTaskType> class ChunkTaskData;
template <ChunkTaskType> class ChunkTaskRunnerData;
template <ChunkTaskType> class ChunkTaskRunner;

template <ChunkTaskType Type> class ChunkTaskDataBase {
private:
	bool m_running{false};
	ChunkTaskPriority m_priority{ChunkTaskPriority::kLow};

	friend class ChunkTaskPool;
	friend class ChunkTaskPoolLocked;

public:
	inline bool NotIdle() const { return static_cast<const ChunkTaskData<Type> *>(this)->IsQueued() || m_running; }
};

} // namespace hc::client

#include "ChunkGenerateTask.inl"
#include "ChunkLightTask.inl"
#include "ChunkMeshTask.inl"
#include "ChunkSetBlockTask.inl"

namespace hc::client {

template <ChunkTaskType TaskType = static_cast<ChunkTaskType>(0)> struct ChunkTaskDataTuple {
	using Type = decltype(std::tuple_cat(
	    std::tuple<ChunkTaskData<TaskType>>{},
	    typename ChunkTaskDataTuple<static_cast<ChunkTaskType>(static_cast<unsigned>(TaskType) + 1)>::Type{}));
};
template <> struct ChunkTaskDataTuple<ChunkTaskType::COUNT> {
	using Type = std::tuple<>;
};

template <ChunkTaskType TaskType = static_cast<ChunkTaskType>(0)> struct ChunkTaskRunnerTuple {
	using Type = decltype(std::tuple_cat(
	    std::tuple<ChunkTaskRunner<TaskType>>{},
	    typename ChunkTaskRunnerTuple<static_cast<ChunkTaskType>(static_cast<unsigned>(TaskType) + 1)>::Type{}));
};
template <> struct ChunkTaskRunnerTuple<ChunkTaskType::COUNT> {
	using Type = std::tuple<>;
};

template <typename, typename> struct VariantCat;
template <typename NewType, typename... VariantTypes> struct VariantCat<NewType, std::variant<VariantTypes...>> {
	using Type = std::variant<NewType, VariantTypes...>;
};

template <ChunkTaskType TaskType = static_cast<ChunkTaskType>(0)> struct ChunkTaskRunnerDataVariant {
	using Type = typename VariantCat<ChunkTaskRunnerData<TaskType>,
	                                 typename ChunkTaskRunnerDataVariant<static_cast<ChunkTaskType>(
	                                     static_cast<unsigned>(TaskType) + 1)>::Type>::Type;
};
template <> struct ChunkTaskRunnerDataVariant<ChunkTaskType::COUNT> {
	using Type = std::variant<std::monostate>;
};

class ChunkTaskPoolToken;
class ChunkTaskPool {
private:
	using DataTuple = typename ChunkTaskDataTuple<>::Type;
	using RunnerDataVariant = typename ChunkTaskRunnerDataVariant<>::Type;

	World &m_world;
	libcuckoo::cuckoohash_map<ChunkPos3, DataTuple> m_data_map;
	moodycamel::ConcurrentQueue<RunnerDataVariant> m_high_priority_runner_data_queue, m_low_priority_runner_data_queue;
	std::atomic_bool m_high_priority_producer_flag;
	std::mutex m_producer_mutex;

	void produce_runner_data(ChunkTaskPoolToken *p_token, std::size_t max_tasks);

	friend class ChunkTaskPoolToken;
	friend class ChunkTaskPoolLocked;

public:
	inline explicit ChunkTaskPool(World *p_world) : m_world{*p_world}, m_high_priority_producer_flag{false} {}

	template <ChunkTaskType TaskType, ChunkTaskPriority TaskPriority = ChunkTaskPriority::kLow, typename... Args>
	inline void Push(const ChunkPos3 &chunk_pos, Args &&...args) {
		m_data_map.uprase_fn(chunk_pos,
		                     [... args = std::forward<Args>(args)](DataTuple &data_tuple, libcuckoo::UpsertContext) {
			                     auto &data = std::get<static_cast<std::size_t>(TaskType)>(data_tuple);
			                     data.Push(args...);
			                     if constexpr (TaskPriority == ChunkTaskPriority::kHigh)
				                     data.m_priority = ChunkTaskPriority::kHigh;
			                     return false;
		                     });
		if constexpr (TaskPriority == ChunkTaskPriority::kHigh)
			m_high_priority_producer_flag.store(true, std::memory_order_release);
	}
	inline void Clear() {
		m_data_map.clear();
		// clear queue
	}

	inline const World &GetWorld() const { return m_world; }
	inline World &GetWorld() { return m_world; }

	inline auto GetPendingTaskCount() const { return m_data_map.size(); }
	inline auto GetRunningTaskCountApprox() const {
		return m_low_priority_runner_data_queue.size_approx() + m_high_priority_runner_data_queue.size_approx();
	}
	void Run(ChunkTaskPoolToken *p_token, std::size_t producer_max_tasks);
};

class ChunkTaskPoolLocked {
private:
	using DataTuple = typename ChunkTaskDataTuple<>::Type;
	using RunnerDataVariant = typename ChunkTaskRunnerDataVariant<>::Type;

	World &m_world;
	libcuckoo::cuckoohash_map<ChunkPos3, DataTuple>::locked_table m_data_map;

public:
	inline explicit ChunkTaskPoolLocked(ChunkTaskPool *p_pool)
	    : m_world{p_pool->m_world}, m_data_map{p_pool->m_data_map.lock_table()} {}
	inline const World &GetWorld() const { return m_world; }
	inline World &GetWorld() { return m_world; }

	template <ChunkTaskType... TaskTypes> inline bool AllNotIdle(const ChunkPos3 &chunk_pos) const {
		auto it = m_data_map.find(chunk_pos);
		return !(it == m_data_map.end()) &&
		       (std::get<static_cast<std::size_t>(TaskTypes)>(it->second).NotIdle() && ...);
	}
	template <ChunkTaskType... TaskTypes> inline bool AnyNotIdle(const ChunkPos3 &chunk_pos) const {
		auto it = m_data_map.find(chunk_pos);
		return !(it == m_data_map.end()) &&
		       (std::get<static_cast<std::size_t>(TaskTypes)>(it->second).NotIdle() || ...);
	}
	template <ChunkTaskType TaskType, typename... Args> inline void Push(const ChunkPos3 &chunk_pos, Args &&...args) {
		auto it = m_data_map.insert(chunk_pos).first;
		auto &data = std::get<static_cast<std::size_t>(TaskType)>(it->second);
		data.Push(args...);
	}
	template <ChunkTaskType TaskType, typename Iterator, typename... Args>
	inline void PushBulk(Iterator chunk_pos_begin, Iterator chunk_pos_end, Args &&...args) {
		for (Iterator it = chunk_pos_begin; it != chunk_pos_end; ++it)
			Push<TaskType>(*it, args...);
	}

	inline const auto &GetDataMap() const { return m_data_map; }
	inline auto &GetDataMap() { return m_data_map; }
};

class ChunkTaskPoolToken {
private:
	using RunnerTuple = typename ChunkTaskRunnerTuple<>::Type;

	moodycamel::ProducerToken m_high_priority_producer_token, m_low_priority_producer_token;
	moodycamel::ConsumerToken m_high_priority_consumer_token, m_low_priority_consumer_token;
	RunnerTuple m_runners;

	friend class ChunkTaskPool;

public:
	inline explicit ChunkTaskPoolToken(ChunkTaskPool *p_pool)
	    : m_high_priority_producer_token{p_pool->m_high_priority_runner_data_queue},
	      m_high_priority_consumer_token{p_pool->m_high_priority_runner_data_queue},
	      m_low_priority_producer_token{p_pool->m_low_priority_runner_data_queue},
	      m_low_priority_consumer_token{p_pool->m_low_priority_runner_data_queue} {}
};

} // namespace hc::client

#endif