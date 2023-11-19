#include <client/Chunk.hpp>

#include <glm/gtx/hash.hpp>
#include <unordered_map>

namespace hc::client {

template <>
class ChunkTaskData<ChunkTaskType::kUpdateBlock> final : public ChunkTaskDataBase<ChunkTaskType::kUpdateBlock> {
private:
	std::unordered_map<InnerIndex3, uint64_t> m_updates;

public:
	inline static constexpr ChunkTaskType kType = ChunkTaskType::kUpdateBlock;

	inline void Push(InnerIndex3 update, uint64_t tick) { m_updates[update] = tick; }
	inline void Push(std::span<const InnerIndex3> updates, uint64_t tick) {
		for (const auto &pos : updates)
			m_updates[pos] = tick;
	}
	inline constexpr ChunkTaskPriority GetPriority() const { return ChunkTaskPriority::kTick; }
	inline bool IsQueued() const { return !m_updates.empty(); }
	std::optional<ChunkTaskRunnerData<ChunkTaskType::kUpdateBlock>> Pop(const ChunkTaskPoolLocked &task_pool,
	                                                                    const ChunkPos3 &chunk_pos);

	inline void OnUnload() { m_updates.clear(); }
};

template <> class ChunkTaskRunnerData<ChunkTaskType::kUpdateBlock> {
private:
	std::array<std::shared_ptr<Chunk>, 27> m_chunk_ptr_array;
	std::vector<InnerIndex3> m_updates;

public:
	inline static constexpr ChunkTaskType kType = ChunkTaskType::kUpdateBlock;

	inline ChunkTaskRunnerData(std::array<std::shared_ptr<Chunk>, 27> &&chunk_ptr_array,
	                           std::vector<InnerIndex3> &&updates)
	    : m_chunk_ptr_array{std::move(chunk_ptr_array)}, m_updates{std::move(updates)} {}
	inline const ChunkPos3 &GetChunkPos() const { return m_chunk_ptr_array[26]->GetPosition(); }
	inline const std::shared_ptr<Chunk> &GetChunkPtr() const { return m_chunk_ptr_array[26]; }
	inline const auto &GetChunkPtrArray() const { return m_chunk_ptr_array; }
	inline const auto &GetUpdates() const { return m_updates; }
};

template <> class ChunkTaskRunner<ChunkTaskType::kUpdateBlock> {
private:
	block::Block m_update_neighbours[block::kBlockUpdateMaxNeighbours],
	    m_update_set_blocks[block::kBlockUpdateMaxNeighbours];
	InnerPos3 m_update_neighbour_pos[block::kBlockUpdateMaxNeighbours];

public:
	inline static constexpr ChunkTaskType kType = ChunkTaskType::kUpdateBlock;

	void Run(ChunkTaskPool *p_task_pool, ChunkTaskRunnerData<ChunkTaskType::kUpdateBlock> &&data);
};

} // namespace hc::client
