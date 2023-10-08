#pragma once

#include <atomic>
#include <blockingconcurrentqueue.h>
#include <cuckoohash_map.hh>
#include <glm/gtx/hash.hpp>
#include <memory>
#include <optional>
#include <tuple>
#include <variant>

#include "Chunk.hpp"

namespace hc::client {

class World;
class ChunkTaskPool;

enum class ChunkTaskType { kGenerate, kMesh, kLight, COUNT };

template <ChunkTaskType> class ChunkTaskData;
template <ChunkTaskType> class ChunkTaskRunnerData;
template <ChunkTaskType> class ChunkTaskRunner;

template <> class ChunkTaskRunnerData<ChunkTaskType::kGenerate> {
private:
	std::shared_ptr<Chunk> m_chunk_ptr;

public:
	inline ChunkTaskRunnerData(std::shared_ptr<Chunk> chunk_ptr) : m_chunk_ptr{std::move(chunk_ptr)} {}
	inline const std::shared_ptr<Chunk> &GetChunkPtr() const { return m_chunk_ptr; }
};

template <> class ChunkTaskData<ChunkTaskType::kGenerate> {
private:
	std::atomic_bool m_queued{false};

public:
	inline void Push() { m_queued.store(true, std::memory_order_relaxed); }
	inline bool IsQueued() const { return m_queued.load(std::memory_order_relaxed); }
	std::optional<ChunkTaskRunnerData<ChunkTaskType::kGenerate>> Pop(const ChunkTaskPool &task_pool,
	                                                                 const std::shared_ptr<World> &world_ptr);
};

template <> class ChunkTaskRunnerData<ChunkTaskType::kMesh> {
private:
	std::shared_ptr<Chunk> m_chunk_ptr;

public:
	inline ChunkTaskRunnerData(std::shared_ptr<Chunk> chunk_ptr) : m_chunk_ptr{std::move(chunk_ptr)} {}
	inline const std::shared_ptr<Chunk> &GetChunkPtr() const { return m_chunk_ptr; }
};

template <> class ChunkTaskData<ChunkTaskType::kMesh> {
private:
	std::atomic_bool m_queued{false};

public:
	inline void Push() { m_queued.store(true, std::memory_order_relaxed); }
	inline bool IsQueued() const { return m_queued.load(std::memory_order_relaxed); }
	std::optional<ChunkTaskRunnerData<ChunkTaskType::kGenerate>> Pop(const ChunkTaskPool &task_pool,
	                                                                 const std::shared_ptr<World> &world_ptr);
};

template <> class ChunkTaskRunnerData<ChunkTaskType::kLight> {
private:
	std::shared_ptr<Chunk> m_chunk_ptr;

public:
	inline ChunkTaskRunnerData(std::shared_ptr<Chunk> chunk_ptr) : m_chunk_ptr{std::move(chunk_ptr)} {}
	inline const std::shared_ptr<Chunk> &GetChunkPtr() const { return m_chunk_ptr; }
};

template <> class ChunkTaskData<ChunkTaskType::kLight> {
private:
	std::atomic_bool m_queued{false};

public:
	inline void Push() { m_queued.store(true, std::memory_order_relaxed); }
	inline bool IsQueued() const { return m_queued.load(std::memory_order_relaxed); }
	std::optional<ChunkTaskRunnerData<ChunkTaskType::kGenerate>> Pop(const ChunkTaskPool &task_pool,
	                                                                 const std::shared_ptr<World> &world_ptr);
};

template <ChunkTaskType TaskType = static_cast<ChunkTaskType>(0)> struct ChunkTaskDataTuple {
	using Type = decltype(std::tuple_cat(
	    std::tuple<ChunkTaskData<TaskType>>{},
	    typename ChunkTaskDataTuple<static_cast<ChunkTaskType>(static_cast<unsigned>(TaskType) + 1)>::Type{}));
};
template <> struct ChunkTaskDataTuple<ChunkTaskType::COUNT> {
	using Type = std::tuple<>;
};

template <typename, typename> struct VariantCat;
template <typename NewType, typename... VariantTypes> struct VariantCat<NewType, std::variant<VariantTypes...>> {
	using Type = std::variant<VariantTypes..., NewType>;
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

	libcuckoo::cuckoohash_map<ChunkPos3, DataTuple> m_data_map;
	moodycamel::BlockingConcurrentQueue<RunnerDataVariant> m_runner_data_queue;

	friend class ChunkTaskPoolToken;

public:
	template <ChunkTaskType TaskType, typename... Args> inline void Push(const ChunkPos3 &chunk_pos, Args &&...args) {
		m_data_map.uprase_fn(chunk_pos,
		                     [... args = std::forward<Args>(args)](DataTuple &data, libcuckoo::UpsertContext) {
			                     std::get<static_cast<std::size_t>(TaskType)>(data).Push(std::forward<Args>(args)...);
		                     });
	}
	inline void Clear() {
		m_data_map.clear();
		// clear queue
	}

	template <ChunkTaskType... TaskTypes> inline bool AllQueued(const ChunkPos3 &chunk_pos) const {
		bool ret{false};
		m_data_map.find_fn(chunk_pos,
		                   [&ret](const DataTuple &data) { ret = (std::get<TaskTypes>(data).IsQueue() && ...); });
		return ret;
	}
	template <ChunkTaskType... TaskTypes> inline bool AnyQueued(const ChunkPos3 &chunk_pos) const {
		bool ret{false};
		m_data_map.find_fn(chunk_pos,
		                   [&ret](const DataTuple &data) { ret = (std::get<TaskTypes>(data).IsQueue() || ...); });
		return ret;
	}
	template <typename Rep, typename Period>
	inline RunnerDataVariant TryPop(ChunkTaskPoolToken *p_token, const std::chrono::duration<Rep, Period> &timeout) {
		RunnerDataVariant runner_data = std::monostate{};
		if (m_runner_data_queue.wait_dequeue_timed(p_token->m_consumer_token, runner_data, timeout)) {
		}
		return runner_data;
	}
};

class ChunkTaskPoolToken {
private:
	moodycamel::ProducerToken m_producer_token;
	moodycamel::ConsumerToken m_consumer_token;

	friend class ChunkTaskPool;

public:
	inline ChunkTaskPoolToken(ChunkTaskPool *p_pool)
	    : m_producer_token{p_pool->m_runner_data_queue}, m_consumer_token{p_pool->m_runner_data_queue} {}
};

} // namespace hc::client