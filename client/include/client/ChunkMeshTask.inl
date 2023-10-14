#include <client/Chunk.hpp>

namespace hc::client {

class WorldRenderer;

template <> class ChunkTaskRunnerData<ChunkTaskType::kMesh> {
private:
	std::array<std::shared_ptr<Chunk>, 27> m_chunk_ptr_array;
	bool m_init_light;

public:
	inline static constexpr ChunkTaskType kType = ChunkTaskType::kMesh;

	inline ChunkTaskRunnerData(std::array<std::shared_ptr<Chunk>, 27> chunk_ptr_array, bool init_light)
	    : m_chunk_ptr_array{std::move(chunk_ptr_array)}, m_init_light{init_light} {}
	inline const ChunkPos3 &GetChunkPos() const { return m_chunk_ptr_array[26]->GetPosition(); }
	inline const std::shared_ptr<Chunk> &GetChunkPtr() const { return m_chunk_ptr_array[26]; }
	inline bool ShouldInitLight() const { return m_init_light; }
	inline const auto &GetChunkPtrArray() const { return m_chunk_ptr_array; }
};

template <> class ChunkTaskData<ChunkTaskType::kMesh> {
private:
	bool m_queued{false}, m_init_light{false};

public:
	inline static constexpr ChunkTaskType kType = ChunkTaskType::kMesh;

	inline void Push(bool init_light = false) {
		m_queued = true;
		m_init_light |= init_light;
	}
	inline bool IsQueued() const { return m_queued; }
	std::optional<ChunkTaskRunnerData<ChunkTaskType::kMesh>> Pop(const ChunkTaskPoolLocked &task_pool,
	                                                             const ChunkPos3 &chunk_pos);
	inline void OnUnload() { m_queued = m_init_light = false; }
};

template <> class ChunkTaskRunner<ChunkTaskType::kMesh> {
private:
	block::Light m_extend_light_buffer[(kChunkSize + 30) * (kChunkSize + 30) * (kChunkSize + 30) -
	                                   kChunkSize * kChunkSize * kChunkSize]{};

public:
	inline static constexpr ChunkTaskType kType = ChunkTaskType::kMesh;

	void Run(ChunkTaskPool *p_task_pool, ChunkTaskRunnerData<ChunkTaskType::kMesh> &&data);
};

} // namespace hc::client
