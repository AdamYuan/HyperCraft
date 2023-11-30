#include <client/Chunk.hpp>
#include <client/ChunkUpdate.hpp>

namespace hc::client {

template <>
class ChunkTaskData<ChunkTaskType::kSetSunlight> final : public ChunkTaskDataBase<ChunkTaskType::kSetSunlight> {
private:
	std::unordered_map<InnerIndex2, ChunkUpdate<InnerPos1>> m_set_sunlight_map;

public:
	inline static constexpr ChunkTaskType kType = ChunkTaskType::kSetSunlight;

	inline void Push(std::span<const ChunkSunlightEntry> set_sunlights, ChunkUpdateType type, bool push_back = true) {
		if (push_back) {
			for (const auto &entry : set_sunlights)
				m_set_sunlight_map[entry.index].Get(type) = entry.sunlight;
		} else {
			for (const auto &entry : set_sunlights) {
				auto &update = m_set_sunlight_map[entry.index];
				if (!update.Get(type).has_value())
					m_set_sunlight_map[entry.index].Get(type) = entry.sunlight;
			}
		}
	}
	inline ChunkTaskPriority GetPriority() const { return ChunkTaskPriority::kLow; }
	inline bool IsQueued() const { return !m_set_sunlight_map.empty(); }
	std::optional<ChunkTaskRunnerData<ChunkTaskType::kSetSunlight>> Pop(const ChunkTaskPoolLocked &task_pool,
	                                                                    const ChunkPos3 &chunk_pos);

	inline void OnUnload() {}
};

template <> class ChunkTaskRunnerData<ChunkTaskType::kSetSunlight> {
private:
	std::shared_ptr<Chunk> m_chunk_ptr;
	std::unordered_map<InnerIndex2, ChunkUpdate<InnerPos1>> m_set_sunlight_map;

public:
	inline static constexpr ChunkTaskType kType = ChunkTaskType::kSetSunlight;

	inline ChunkTaskRunnerData(std::shared_ptr<Chunk> chunk_ptr,
	                           std::unordered_map<InnerIndex2, ChunkUpdate<InnerPos1>> &&set_sunlight_map)
	    : m_chunk_ptr{std::move(chunk_ptr)}, m_set_sunlight_map{std::move(set_sunlight_map)} {}
	inline const ChunkPos3 &GetChunkPos() const { return m_chunk_ptr->GetPosition(); }
	inline const std::shared_ptr<Chunk> &GetChunkPtr() const { return m_chunk_ptr; }
	inline const auto &GetSetSunlightMap() const { return m_set_sunlight_map; }
};

template <> class ChunkTaskRunner<ChunkTaskType::kSetSunlight> {
private:
public:
	inline static constexpr ChunkTaskType kType = ChunkTaskType::kSetSunlight;

	void Run(ChunkTaskPool *p_task_pool, ChunkTaskRunnerData<ChunkTaskType::kSetSunlight> &&data);
};

} // namespace hc::client
