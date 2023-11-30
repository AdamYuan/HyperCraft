#include <client/Chunk.hpp>
#include <client/ChunkUpdate.hpp>

#include <common/Data.hpp>

namespace hc::client {

class ClientBase;

template <> class ChunkTaskData<ChunkTaskType::kSetBlock> final : public ChunkTaskDataBase<ChunkTaskType::kSetBlock> {
private:
	std::unordered_map<InnerIndex3, ChunkUpdate<block::Block>> m_set_block_map;

public:
	inline static constexpr ChunkTaskType kType = ChunkTaskType::kSetBlock;

	inline void Push(std::span<const ChunkBlockEntry> set_blocks, ChunkUpdateType type, bool push_back = true) {
		if (push_back) {
			for (const auto &entry : set_blocks)
				m_set_block_map[entry.index].Get(type) = entry.block;
		} else {
			for (const auto &entry : set_blocks) {
				auto &update = m_set_block_map[entry.index];
				if (!update.Get(type).has_value())
					m_set_block_map[entry.index].Get(type) = entry.block;
			}
		}
	}
	inline ChunkTaskPriority GetPriority() const { return ChunkTaskPriority::kHigh; }
	inline bool IsQueued() const { return !m_set_block_map.empty(); }
	std::optional<ChunkTaskRunnerData<ChunkTaskType::kSetBlock>> Pop(const ChunkTaskPoolLocked &task_pool,
	                                                                 const ChunkPos3 &chunk_pos);

	inline void OnUnload() {}
};

template <> class ChunkTaskRunnerData<ChunkTaskType::kSetBlock> {
private:
	std::shared_ptr<Chunk> m_chunk_ptr;
	std::unordered_map<InnerIndex3, ChunkUpdate<block::Block>> m_set_block_map;

public:
	inline static constexpr ChunkTaskType kType = ChunkTaskType::kSetBlock;
	inline ChunkTaskRunnerData(std::shared_ptr<Chunk> chunk_ptr,
	                           std::unordered_map<InnerIndex3, ChunkUpdate<block::Block>> &&set_block_map)
	    : m_chunk_ptr{std::move(chunk_ptr)}, m_set_block_map{std::move(set_block_map)} {}
	inline const ChunkPos3 &GetChunkPos() const { return m_chunk_ptr->GetPosition(); }
	inline const std::shared_ptr<Chunk> &GetChunkPtr() const { return m_chunk_ptr; }
	inline const auto &GetSetBlockMap() const { return m_set_block_map; }
};

template <> class ChunkTaskRunner<ChunkTaskType::kSetBlock> {
public:
	inline static constexpr ChunkTaskType kType = ChunkTaskType::kSetBlock;

	static void Run(ChunkTaskPool *p_task_pool, ChunkTaskRunnerData<ChunkTaskType::kSetBlock> &&data);
};

} // namespace hc::client
