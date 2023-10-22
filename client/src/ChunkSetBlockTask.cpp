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
		auto pos = block_change.first;
		auto idx = Chunk::XYZ2Index(pos.x, pos.y, pos.z);
		auto new_block = block_change.second, old_block = chunk->GetBlock(idx);
		if (new_block == old_block)
			continue;
		if (new_block.GetVerticalLightPass() != old_block.GetVerticalLightPass()) {
			// Push Light Update
		}
		chunk->SetBlock(idx, new_block);
		neighbour_remesh_set[CmpXYZ2NeighbourIndex(pos.x == 0 ? -1 : (pos.x == kChunkSize - 1 ? 1 : 0),
		                                           pos.y == 0 ? -1 : (pos.y == kChunkSize - 1 ? 1 : 0),
		                                           pos.z == 0 ? -1 : (pos.z == kChunkSize - 1 ? 1 : 0))] = true;
	}

	for (uint32_t i = 0; i < 27; ++i) {
		if (!neighbour_remesh_set[i])
			continue;

		ChunkPos3 nei_pos;
		Chunk::NeighbourIndex2CmpXYZ(i, glm::value_ptr(nei_pos));
		nei_pos += chunk->GetPosition();
		p_task_pool->Push<ChunkTaskType::kMesh, ChunkTaskPriority::kHigh>(nei_pos, false);
	}
}

} // namespace hc::client