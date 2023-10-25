#include <client/Chunk.hpp>

#include <client/BlockLightAlgo.hpp>

namespace hc::client {

class WorldRenderer;

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
	bool m_queued{false};

public:
	inline static constexpr ChunkTaskType kType = ChunkTaskType::kMesh;

	inline void Push() { m_queued = true; }
	inline bool IsQueued() const { return m_queued; }
	std::optional<ChunkTaskRunnerData<ChunkTaskType::kMesh>> Pop(const ChunkTaskPoolLocked &task_pool,
	                                                             const ChunkPos3 &chunk_pos);
	inline void OnUnload() { m_queued = false; }
};

template <> class ChunkTaskRunner<ChunkTaskType::kMesh> {
private:
	using LightAlgo = BlockLightAlgo<
	    BlockAlgoConfig<int32_t, BlockAlgoBound<int32_t>{-1, -1, -1, (int32_t)kChunkSize + 1, (int32_t)kChunkSize + 1,
	                                                     (int32_t)kChunkSize + 1}>,
	    14>;
	LightAlgo::Queue m_sunlight_entries, m_torchlight_entries;
	block::Light m_extend_light_buffer[(kChunkSize + 30) * (kChunkSize + 30) * (kChunkSize + 30) -
	                                   kChunkSize * kChunkSize * kChunkSize]{};

public:
	inline static constexpr ChunkTaskType kType = ChunkTaskType::kMesh;

	void Run(ChunkTaskPool *p_task_pool, ChunkTaskRunnerData<ChunkTaskType::kMesh> &&data);
};

} // namespace hc::client
