#include <client/Chunk.hpp>

namespace hc::client {

class ClientBase;

template <> class ChunkTaskRunnerData<ChunkTaskType::kGenerate> {
private:
	std::shared_ptr<Chunk> m_chunk_ptr;

public:
	inline static constexpr ChunkTaskType kType = ChunkTaskType::kGenerate;
	inline ChunkTaskRunnerData(std::shared_ptr<Chunk> chunk_ptr) : m_chunk_ptr{std::move(chunk_ptr)} {}
	inline const ChunkPos3 &GetChunkPos() const { return m_chunk_ptr->GetPosition(); }
	inline const std::shared_ptr<Chunk> &GetChunkPtr() const { return m_chunk_ptr; }
};

template <> class ChunkTaskData<ChunkTaskType::kGenerate> final : public ChunkTaskDataBase<ChunkTaskType::kGenerate> {
private:
	bool m_queued{false};

public:
	inline static constexpr ChunkTaskType kType = ChunkTaskType::kGenerate;

	inline void Push() { m_queued = true; }
	inline constexpr ChunkTaskPriority GetPriority() const { return ChunkTaskPriority::kLow; }
	inline bool IsQueued() const { return m_queued; }
	std::optional<ChunkTaskRunnerData<ChunkTaskType::kGenerate>> Pop(const ChunkTaskPoolLocked &task_pool,
	                                                                 const ChunkPos3 &chunk_pos);

	inline void OnUnload() { m_queued = false; }
};

template <> class ChunkTaskRunner<ChunkTaskType::kGenerate> {
public:
	inline static constexpr ChunkTaskType kType = ChunkTaskType::kGenerate;

	void Run(ChunkTaskPool *p_task_pool, ChunkTaskRunnerData<ChunkTaskType::kGenerate> &&data);
};

} // namespace hc::client
