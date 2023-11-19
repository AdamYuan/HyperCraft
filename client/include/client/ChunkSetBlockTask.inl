#include <client/Chunk.hpp>
#include <client/ChunkUpdate.hpp>

namespace hc::client {

class ClientBase;

template <> class ChunkTaskData<ChunkTaskType::kSetBlock> final : public ChunkTaskDataBase<ChunkTaskType::kSetBlock> {
private:
	std::vector<ChunkSetBlock> m_set_blocks;
	bool m_high_priority{false};

public:
	inline static constexpr ChunkTaskType kType = ChunkTaskType::kSetBlock;

	inline void Push(ChunkSetBlock set_block, bool high_priority = false) {
		m_set_blocks.emplace_back(set_block);
		m_high_priority |= high_priority;
	}
	inline void Push(std::span<const ChunkSetBlock> set_blocks, bool high_priority = false) {
		m_set_blocks.insert(m_set_blocks.end(), set_blocks.begin(), set_blocks.end());
		m_high_priority |= high_priority;
	}
	inline ChunkTaskPriority GetPriority() const {
		return m_high_priority ? ChunkTaskPriority::kHigh : ChunkTaskPriority::kLow;
	}
	inline bool IsQueued() const { return !m_set_blocks.empty(); }
	std::optional<ChunkTaskRunnerData<ChunkTaskType::kSetBlock>> Pop(const ChunkTaskPoolLocked &task_pool,
	                                                                 const ChunkPos3 &chunk_pos);

	inline void OnUnload() {}
};

template <> class ChunkTaskRunnerData<ChunkTaskType::kSetBlock> {
private:
	std::shared_ptr<Chunk> m_chunk_ptr;
	std::vector<ChunkSetBlock> m_set_blocks;
	bool m_high_priority{false};

public:
	inline static constexpr ChunkTaskType kType = ChunkTaskType::kSetBlock;
	inline ChunkTaskRunnerData(std::shared_ptr<Chunk> chunk_ptr, std::vector<ChunkSetBlock> &&set_blocks,
	                           bool high_priority)
	    : m_chunk_ptr{std::move(chunk_ptr)}, m_set_blocks{std::move(set_blocks)}, m_high_priority{high_priority} {}
	inline bool IsHighPriority() const { return m_high_priority; }
	inline const ChunkPos3 &GetChunkPos() const { return m_chunk_ptr->GetPosition(); }
	inline const std::shared_ptr<Chunk> &GetChunkPtr() const { return m_chunk_ptr; }
	inline const auto &GetSetBlocks() const { return m_set_blocks; }
};

template <> class ChunkTaskRunner<ChunkTaskType::kSetBlock> {
public:
	inline static constexpr ChunkTaskType kType = ChunkTaskType::kSetBlock;

	static void Run(ChunkTaskPool *p_task_pool, ChunkTaskRunnerData<ChunkTaskType::kSetBlock> &&data);
};

} // namespace hc::client
