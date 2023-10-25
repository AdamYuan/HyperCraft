#include <client/ChunkTaskPool.hpp>

#include <client/World.hpp>

#include <bitset>

namespace hc::client {

std::optional<ChunkTaskRunnerData<ChunkTaskType::kSetBlock>>
ChunkTaskData<ChunkTaskType::kSetBlock>::Pop(const ChunkTaskPoolLocked &task_pool, const ChunkPos3 &chunk_pos) {
	if (m_set_blocks.empty())
		return std::nullopt;

	if (task_pool.AnyNotIdle<ChunkTaskType::kGenerate>(chunk_pos))
		return std::nullopt;
	std::shared_ptr<Chunk> chunk;
	if (!(chunk = task_pool.GetWorld().GetChunkPool().FindChunk(chunk_pos)))
		return std::nullopt;
	auto set_blocks = std::move(m_set_blocks);
	m_set_blocks.clear();

	return ChunkTaskRunnerData<ChunkTaskType::kSetBlock>{std::move(chunk), std::move(set_blocks)};
}

void ChunkTaskRunner<ChunkTaskType::kSetBlock>::Run(ChunkTaskPool *p_task_pool,
                                                    ChunkTaskRunnerData<ChunkTaskType::kSetBlock> &&data) {
	std::unordered_map<InnerPos3, block::Block> block_changes;
	for (const auto &set_block : data.GetSetBlocks())
		block_changes[set_block.first] = set_block.second;

	const auto &chunk = data.GetChunkPtr();

	std::bitset<27> neighbour_remesh_set{};
	neighbour_remesh_set[26] = true;

	for (const auto &block_change : block_changes) {
		auto block_pos = block_change.first;
		auto block_idx = Chunk::XYZ2Index(block_pos.x, block_pos.y, block_pos.z);
		auto new_block = block_change.second, old_block = chunk->GetBlock(block_idx);
		if (new_block == old_block)
			continue;
		chunk->SetBlock(block_idx, new_block);

		for (uint32_t i = 0; i < 26; ++i) {
			if (neighbour_remesh_set[i])
				continue;
			ChunkPos3 nei_chunk_pos;
			Chunk::NeighbourIndex2CmpXYZ(i, glm::value_ptr(nei_chunk_pos));
			neighbour_remesh_set[i] = ChunkShouldRemesh(block_pos - InnerPos3(nei_chunk_pos) * InnerPos1(kChunkSize));
		}
	}

	for (uint32_t i = 26; ~i; --i) {
		if (!neighbour_remesh_set[i])
			continue;
		ChunkPos3 nei_pos;
		Chunk::NeighbourIndex2CmpXYZ(i, glm::value_ptr(nei_pos));
		nei_pos += chunk->GetPosition();
		p_task_pool->Push<ChunkTaskType::kMesh, ChunkTaskPriority::kHigh>(nei_pos);
	}
}

} // namespace hc::client