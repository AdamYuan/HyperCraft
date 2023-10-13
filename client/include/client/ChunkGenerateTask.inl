#include <client/Chunk.hpp>

namespace hc::client {

class ClientBase;

template <> class ChunkTaskRunnerData<ChunkTaskType::kGenerate> {
private:
	std::shared_ptr<Chunk> m_chunk_ptr;
	std::shared_ptr<ClientBase> m_client_ptr;

public:
	inline static constexpr ChunkTaskType kType = ChunkTaskType::kGenerate;
	inline ChunkTaskRunnerData(std::shared_ptr<Chunk> chunk_ptr, std::shared_ptr<ClientBase> client_ptr)
	    : m_chunk_ptr{std::move(chunk_ptr)}, m_client_ptr{std::move(client_ptr)} {}
	inline const ChunkPos3 &GetChunkPos() const { return m_chunk_ptr->GetPosition(); }
	inline const std::shared_ptr<Chunk> &GetChunkPtr() const { return m_chunk_ptr; }
	inline const std::shared_ptr<ClientBase> &GetClientPtr() const { return m_client_ptr; }
};

template <> class ChunkTaskData<ChunkTaskType::kGenerate> {
private:
	bool m_queued{false};

public:
	inline static constexpr ChunkTaskType kType = ChunkTaskType::kGenerate;

	inline void Push() { m_queued = true; }
	inline bool IsQueued() const { return m_queued; }
	std::optional<ChunkTaskRunnerData<ChunkTaskType::kGenerate>> Pop(const ChunkTaskPool &task_pool,
	                                                                 const ChunkPos3 &chunk_pos);
};

template <> class ChunkTaskRunner<ChunkTaskType::kGenerate> {
private:
	int32_t m_light_map[kChunkSize * kChunkSize];

public:
	inline static constexpr ChunkTaskType kType = ChunkTaskType::kGenerate;

	void Run(ChunkTaskPool *p_task_pool, ChunkTaskRunnerData<ChunkTaskType::kGenerate> &&data);
};

} // namespace hc::client
