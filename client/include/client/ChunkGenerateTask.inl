#include <client/Chunk.hpp>

#include <common/Data.hpp>

namespace hc::client {

class ClientBase;

template <> class ChunkTaskRunnerData<ChunkTaskType::kGenerate> {
private:
	std::shared_ptr<Chunk> m_chunk_ptr;
	PackedChunkEntry m_chunk_entry;

public:
	inline static constexpr ChunkTaskType kType = ChunkTaskType::kGenerate;
	inline ChunkTaskRunnerData(std::shared_ptr<Chunk> chunk_ptr, PackedChunkEntry &&chunk_entry)
	    : m_chunk_ptr{std::move(chunk_ptr)}, m_chunk_entry{std::move(chunk_entry)} {}
	inline const ChunkPos3 &GetChunkPos() const { return m_chunk_ptr->GetPosition(); }
	inline const std::shared_ptr<Chunk> &GetChunkPtr() const { return m_chunk_ptr; }
	inline const PackedChunkEntry &GetChunkEntry() const { return m_chunk_entry; }
};

template <> class ChunkTaskData<ChunkTaskType::kGenerate> final : public ChunkTaskDataBase<ChunkTaskType::kGenerate> {
private:
	bool m_queued{false};
	PackedChunkEntry m_chunk_entry;

public:
	inline static constexpr ChunkTaskType kType = ChunkTaskType::kGenerate;

	inline void Push(PackedChunkEntry &&chunk_entry) {
		m_chunk_entry = std::move(chunk_entry);
		m_queued = true;
	}
	inline constexpr ChunkTaskPriority GetPriority() const { return ChunkTaskPriority::kLow; }
	inline bool IsQueued() const { return m_queued; }
	std::optional<ChunkTaskRunnerData<ChunkTaskType::kGenerate>> Pop(const ChunkTaskPoolLocked &task_pool,
	                                                                 const ChunkPos3 &chunk_pos);

	inline void OnUnload() {
		m_chunk_entry.sunlights.clear();
		m_chunk_entry.sunlights.shrink_to_fit();
		m_chunk_entry.blocks.clear();
		m_chunk_entry.blocks.shrink_to_fit();
		m_queued = false;
	}
};

template <> class ChunkTaskRunner<ChunkTaskType::kGenerate> {
public:
	inline static constexpr ChunkTaskType kType = ChunkTaskType::kGenerate;

	void Run(ChunkTaskPool *p_task_pool, ChunkTaskRunnerData<ChunkTaskType::kGenerate> &&data);
};

} // namespace hc::client
