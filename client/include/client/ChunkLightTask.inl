#include <client/Chunk.hpp>

namespace hc::client {

template <> class ChunkTaskRunnerData<ChunkTaskType::kLight> {
private:
	std::shared_ptr<Chunk> m_chunk_ptr;

public:
	inline static constexpr ChunkTaskType kType = ChunkTaskType::kLight;

	inline ChunkTaskRunnerData(std::shared_ptr<Chunk> chunk_ptr) : m_chunk_ptr{std::move(chunk_ptr)} {}
	inline const ChunkPos3 &GetChunkPos() const { return m_chunk_ptr->GetPosition(); }
	inline const std::shared_ptr<Chunk> &GetChunkPtr() const { return m_chunk_ptr; }
};

template <> class ChunkTaskData<ChunkTaskType::kLight> final : public ChunkTaskDataBase<ChunkTaskType::kLight> {
private:
	bool m_queued{false};

public:
	inline static constexpr ChunkTaskType kType = ChunkTaskType::kLight;

	inline void Push() { m_queued = true; }
	inline bool IsQueued() const { return m_queued; }
	std::optional<ChunkTaskRunnerData<ChunkTaskType::kLight>> Pop(const ChunkTaskPoolLocked &task_pool,
	                                                              const ChunkPos3 &chunk_pos) {
		return std::nullopt;
	}
	inline void OnUnload() {}
};

template <> class ChunkTaskRunner<ChunkTaskType::kLight> {
private:
public:
	inline static constexpr ChunkTaskType kType = ChunkTaskType::kLight;

	void Run(ChunkTaskPool *p_task_pool, ChunkTaskRunnerData<ChunkTaskType::kLight> &&data) {}
};

} // namespace hc::client
