#include <client/Chunk.hpp>

#include <client/BlockLightAlgo.hpp>

namespace hc::client {

class WorldRenderer;

inline constexpr bool ChunkShouldRemesh(InnerPos3 relative_block_pos) {
	using LightAlgo = BlockLightAlgo<
	    BlockAlgoConfig<InnerPos1, BlockAlgoBound<InnerPos1>{-1, -1, -1, (InnerPos1)kChunkSize + 1,
	                                                         (InnerPos1)kChunkSize + 1, (InnerPos1)kChunkSize + 1}>,
	    14>;
	return LightAlgo::IsBorderLightInterfere(relative_block_pos.x, relative_block_pos.y, relative_block_pos.z, 15);
}

template <> class ChunkTaskRunnerData<ChunkTaskType::kMesh> {
private:
	std::array<std::shared_ptr<Chunk>, 27> m_chunk_ptr_array;

public:
	inline static constexpr ChunkTaskType kType = ChunkTaskType::kMesh;

	inline ChunkTaskRunnerData(std::array<std::shared_ptr<Chunk>, 27> &&chunk_ptr_array)
	    : m_chunk_ptr_array{std::move(chunk_ptr_array)} {}
	inline const ChunkPos3 &GetChunkPos() const { return m_chunk_ptr_array[26]->GetPosition(); }
	inline const std::shared_ptr<Chunk> &GetChunkPtr() const { return m_chunk_ptr_array[26]; }
	inline const auto &GetChunkPtrArray() const { return m_chunk_ptr_array; }
};

template <> class ChunkTaskData<ChunkTaskType::kMesh> final : public ChunkTaskDataBase<ChunkTaskType::kMesh> {
private:
	bool m_queued{false}, m_high_priority{false};

public:
	inline static constexpr ChunkTaskType kType = ChunkTaskType::kMesh;

	inline void Push(bool high_priority = false) {
		m_queued = true;
		m_high_priority |= high_priority;
	}
	inline ChunkTaskPriority GetPriority() const {
		return m_high_priority ? ChunkTaskPriority::kHigh : ChunkTaskPriority::kLow;
	}
	inline bool IsQueued() const { return m_queued; }
	std::optional<ChunkTaskRunnerData<ChunkTaskType::kMesh>> Pop(const ChunkTaskPoolLocked &task_pool,
	                                                             const ChunkPos3 &chunk_pos);
	inline void OnUnload() { m_queued = false; }
};

template <> class ChunkTaskRunner<ChunkTaskType::kMesh> {
private:
	using LightAlgo = BlockLightAlgo<
	    BlockAlgoConfig<InnerPos1, BlockAlgoBound<InnerPos1>{-1, -1, -1, (InnerPos1)kChunkSize + 1,
	                                                         (InnerPos1)kChunkSize + 1, (InnerPos1)kChunkSize + 1}>,
	    14>;
	LightAlgo::Queue m_sunlight_entries, m_torchlight_entries;
	block::Light m_extend_light_buffer[(kChunkSize + 30) * (kChunkSize + 30) * (kChunkSize + 30)]{};

public:
	inline static constexpr ChunkTaskType kType = ChunkTaskType::kMesh;

	void Run(ChunkTaskPool *p_task_pool, ChunkTaskRunnerData<ChunkTaskType::kMesh> &&data);
};

} // namespace hc::client
