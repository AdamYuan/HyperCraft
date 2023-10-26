#include <client/Chunk.hpp>

namespace hc::client {

class ClientBase;

template <> class ChunkTaskData<ChunkTaskType::kSetBlock> final : public ChunkTaskDataBase<ChunkTaskType::kSetBlock> {
private:
	std::vector<std::pair<InnerPos3, block::Block>> m_set_blocks;

public:
	inline static constexpr ChunkTaskType kType = ChunkTaskType::kSetBlock;

	inline void Push(InnerPos3 inner_pos, block::Block block) { m_set_blocks.emplace_back(inner_pos, block); }
	inline void Push(std::span<std::pair<InnerPos3, block::Block>> set_blocks) {
		m_set_blocks.insert(m_set_blocks.end(), set_blocks.begin(), set_blocks.end());
	}
	inline bool IsQueued() const { return !m_set_blocks.empty(); }
	std::optional<ChunkTaskRunnerData<ChunkTaskType::kSetBlock>> Pop(const ChunkTaskPoolLocked &task_pool,
	                                                                 const ChunkPos3 &chunk_pos);

	inline void OnUnload() {}
};

template <> class ChunkTaskRunnerData<ChunkTaskType::kSetBlock> {
private:
	std::shared_ptr<Chunk> m_chunk_ptr;
	std::vector<std::pair<InnerPos3, block::Block>> m_set_blocks;

public:
	inline static constexpr ChunkTaskType kType = ChunkTaskType::kSetBlock;
	inline ChunkTaskRunnerData(std::shared_ptr<Chunk> chunk_ptr,
	                           std::vector<std::pair<InnerPos3, block::Block>> &&set_blocks)
	    : m_chunk_ptr{std::move(chunk_ptr)}, m_set_blocks{std::move(set_blocks)} {}
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
