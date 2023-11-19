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
	bool high_priority = m_high_priority;
	m_high_priority = false;
	return ChunkTaskRunnerData<ChunkTaskType::kSetBlock>{std::move(chunk), std::move(set_blocks), high_priority};
}

void ChunkTaskRunner<ChunkTaskType::kSetBlock>::Run(ChunkTaskPool *p_task_pool,
                                                    ChunkTaskRunnerData<ChunkTaskType::kSetBlock> &&data) {
	std::unordered_map<InnerIndex3, block::Block> block_changes;
	for (const auto &set_block : data.GetSetBlocks())
		block_changes[set_block.index] = set_block.block;

	// TODO: trigger server event for local changes

	const auto &chunk = data.GetChunkPtr();

	std::bitset<27> neighbour_remesh_set{};

	std::unordered_set<InnerPos2> flood_sunlights;
	std::unordered_set<InnerPos3> block_updates[27];

	for (const auto &block_change : block_changes) {
		auto block_idx = block_change.first;
		auto block_pos = ChunkIndex2XYZ(block_idx);
		auto new_block = block_change.second, old_block = chunk->GetBlock(block_idx);
		if (new_block == old_block)
			continue;

		chunk->SetBlock(block_idx, new_block);

		const auto register_block_update = [&block_updates](InnerPos3 pos) {
			auto [rel_chunk_pos, inner_pos] = ChunkInnerPosFromBlockPos(BlockPos3(pos));
			uint32_t chunk_idx = CmpXYZ2NeighbourIndex(rel_chunk_pos.x, rel_chunk_pos.y, rel_chunk_pos.z);
			block_updates[chunk_idx].insert(inner_pos);
		};
		register_block_update(block_pos);
		register_block_update({block_pos.x - 1, block_pos.y, block_pos.z});
		register_block_update({block_pos.x + 1, block_pos.y, block_pos.z});
		register_block_update({block_pos.x, block_pos.y - 1, block_pos.z});
		register_block_update({block_pos.x, block_pos.y + 1, block_pos.z});
		register_block_update({block_pos.x, block_pos.y, block_pos.z - 1});
		register_block_update({block_pos.x, block_pos.y, block_pos.z + 1});

		flood_sunlights.emplace(block_pos.x, block_pos.z);

		neighbour_remesh_set[26] = true;
		for (uint32_t i = 0; i < 26; ++i) {
			if (neighbour_remesh_set[i])
				continue;
			ChunkPos3 nei_chunk_pos;
			Chunk::NeighbourIndex2CmpXYZ(i, glm::value_ptr(nei_chunk_pos));
			neighbour_remesh_set[i] = ChunkShouldRemesh(block_pos - InnerPos3(nei_chunk_pos) * InnerPos1(kChunkSize));
		}
	}

	auto current_tick = p_task_pool->GetWorld().GetCurrentTick();
	for (uint32_t i = 0; i < 27; ++i) {
		ChunkPos3 nei_pos;
		Chunk::NeighbourIndex2CmpXYZ(i, glm::value_ptr(nei_pos));
		nei_pos += chunk->GetPosition();
		// Trigger remesh
		if (neighbour_remesh_set[i])
			p_task_pool->Push<ChunkTaskType::kMesh>(nei_pos, data.IsHighPriority());
		// Trigger block update
		if (!block_updates[i].empty()) {
			p_task_pool->Push<ChunkTaskType::kUpdateBlock>(
			    nei_pos, std::vector<InnerPos3>(block_updates[i].begin(), block_updates[i].end()), current_tick);
		}
	}

	// Trigger sunlight flood
	auto flood_sunlight_vec = std::vector<InnerPos2>{flood_sunlights.begin(), flood_sunlights.end()};
	p_task_pool->Push<ChunkTaskType::kFloodSunlight>(chunk->GetPosition(), std::span{flood_sunlight_vec}, true);
}

} // namespace hc::client