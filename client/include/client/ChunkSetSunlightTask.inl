#include <client/Chunk.hpp>

namespace hc::client {

template <>
class ChunkTaskData<ChunkTaskType::kSetSunlight> final : public ChunkTaskDataBase<ChunkTaskType::kSetSunlight> {
private:
	std::vector<std::pair<InnerPos2, InnerPos1>> m_set_sunlights;

public:
	inline static constexpr ChunkTaskType kType = ChunkTaskType::kSetSunlight;

	inline void Push(InnerPos2 inner_pos, InnerPos1 sunlight_height) {
		m_set_sunlights.emplace_back(inner_pos, sunlight_height);
	}
	inline void Push(std::span<const std::pair<InnerPos2, InnerPos1>> set_sunlights) {
		m_set_sunlights.insert(m_set_sunlights.end(), set_sunlights.begin(), set_sunlights.end());
	}
	inline constexpr ChunkTaskPriority GetPriority() const { return ChunkTaskPriority::kLow; }
	inline bool IsQueued() const { return !m_set_sunlights.empty(); }
	std::optional<ChunkTaskRunnerData<ChunkTaskType::kSetSunlight>> Pop(const ChunkTaskPoolLocked &task_pool,
	                                                                    const ChunkPos3 &chunk_pos);

	inline void OnUnload() {}
};

template <> class ChunkTaskRunnerData<ChunkTaskType::kSetSunlight> {
private:
	std::shared_ptr<Chunk> m_chunk_ptr;
	std::vector<std::pair<InnerPos2, InnerPos1>> m_set_sunlights;

public:
	inline static constexpr ChunkTaskType kType = ChunkTaskType::kSetSunlight;

	inline ChunkTaskRunnerData(std::shared_ptr<Chunk> chunk_ptr,
	                           std::vector<std::pair<InnerPos2, InnerPos1>> &&set_sunlights)
	    : m_chunk_ptr{std::move(chunk_ptr)}, m_set_sunlights{std::move(set_sunlights)} {}
	inline const ChunkPos3 &GetChunkPos() const { return m_chunk_ptr->GetPosition(); }
	inline const std::shared_ptr<Chunk> &GetChunkPtr() const { return m_chunk_ptr; }
	inline const auto &GetSetSunlights() const { return m_set_sunlights; }
};

template <> class ChunkTaskRunner<ChunkTaskType::kSetSunlight> {
private:
public:
	inline static constexpr ChunkTaskType kType = ChunkTaskType::kSetSunlight;

	void Run(ChunkTaskPool *p_task_pool, ChunkTaskRunnerData<ChunkTaskType::kSetSunlight> &&data);
};

} // namespace hc::client
