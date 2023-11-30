#include <client/ChunkTaskPool.hpp>

#include <client/ClientBase.hpp>
#include <client/World.hpp>

#include <bitset>

namespace hc::client {

std::optional<ChunkTaskRunnerData<ChunkTaskType::kSetBlock>>
ChunkTaskData<ChunkTaskType::kSetBlock>::Pop(const ChunkTaskPoolLocked &task_pool, const ChunkPos3 &chunk_pos) {
	if (m_set_block_map.empty())
		return std::nullopt;

	if (task_pool.AnyNotIdle<ChunkTaskType::kGenerate>(chunk_pos))
		return std::nullopt;

	// if any chunks are being updated around, postpone
	if (task_pool.AnyRunning<ChunkTaskType::kUpdateBlock, ChunkTaskType::kFloodSunlight>(chunk_pos))
		return std::nullopt;
	for (uint32_t i = 0; i < 26; ++i) {
		ChunkPos3 nei_pos;
		Chunk::NeighbourIndex2CmpXYZ(i, glm::value_ptr(nei_pos));
		nei_pos += chunk_pos;
		if (task_pool.AnyRunning<ChunkTaskType::kUpdateBlock>(nei_pos))
			return std::nullopt;
	}

	std::shared_ptr<Chunk> chunk;
	if (!(chunk = task_pool.GetWorld().GetChunkPool().FindChunk(chunk_pos)))
		return std::nullopt;
	auto set_block_map = std::move(m_set_block_map);
	m_set_block_map.clear();
	return ChunkTaskRunnerData<ChunkTaskType::kSetBlock>{std::move(chunk), std::move(set_block_map)};
}

void ChunkTaskRunner<ChunkTaskType::kSetBlock>::Run(ChunkTaskPool *p_task_pool,
                                                    ChunkTaskRunnerData<ChunkTaskType::kSetBlock> &&data) {
	std::shared_ptr<ClientBase> client = p_task_pool->GetWorld().LockClient();
	if (!client)
		return;

	const auto &chunk = data.GetChunkPtr();

	std::bitset<27> neighbour_remesh_set{};

	std::unordered_set<InnerIndex2> flood_sunlights;
	std::unordered_set<InnerIndex3> block_updates[27], block_activates[27];

	std::vector<ChunkSetBlockEntry> client_set_blocks;
	const auto foreach_neighbours_7 = [](InnerPos3 pos, auto func) {
		const auto visit_neighbour = [&func](InnerPos3 pos) {
			auto [rel_chunk_pos, inner_pos] = ChunkInnerPosFromBlockPos(BlockPos3(pos));
			uint32_t chunk_idx = CmpXYZ2NeighbourIndex(rel_chunk_pos.x, rel_chunk_pos.y, rel_chunk_pos.z);
			func(chunk_idx, InnerIndex3FromPos(inner_pos));
		};
		visit_neighbour(pos);
		visit_neighbour({pos.x - 1, pos.y, pos.z});
		visit_neighbour({pos.x + 1, pos.y, pos.z});
		visit_neighbour({pos.x, pos.y - 1, pos.z});
		visit_neighbour({pos.x, pos.y + 1, pos.z});
		visit_neighbour({pos.x, pos.y, pos.z - 1});
		visit_neighbour({pos.x, pos.y, pos.z + 1});
	};

	// place blocks, register block updates and sunlight floods
	for (const auto &it : data.GetSetBlockMap()) {
		const auto &update = it.second;

		auto block_idx = it.first;
		auto block_pos = InnerPos3FromIndex(block_idx);

		if (update.Get<ChunkUpdateType::kRemote>().has_value()) {
			foreach_neighbours_7(block_pos, [&block_activates](uint32_t chunk_idx, InnerIndex3 block_idx) {
				block_activates[chunk_idx].insert(block_idx);
			});
		}

		auto new_block = update.GetNew();
		auto opt_old_block = update.GetOld();
		auto local_old_block = chunk->GetBlock(block_idx);

		if (update.Get<ChunkUpdateType::kLocal>().has_value())
			client_set_blocks.push_back(
			    {.index = block_idx,
			     .old_block = opt_old_block.has_value() ? opt_old_block.value() : local_old_block,
			     .new_block = new_block});

		if (new_block == local_old_block)
			continue;

		chunk->SetBlock(block_idx, new_block);

		foreach_neighbours_7(block_pos, [&block_updates](uint32_t chunk_idx, InnerIndex3 block_idx) {
			block_updates[chunk_idx].insert(block_idx);
		});

		flood_sunlights.emplace(InnerIndex2FromPos(block_pos.x, block_pos.z));

		neighbour_remesh_set[26] = true;
		for (uint32_t i = 0; i < 26; ++i) {
			if (neighbour_remesh_set[i])
				continue;
			ChunkPos3 nei_chunk_pos;
			Chunk::NeighbourIndex2CmpXYZ(i, glm::value_ptr(nei_chunk_pos));
			neighbour_remesh_set[i] = ChunkShouldRemesh(block_pos - InnerPos3(nei_chunk_pos) * InnerPos1(kChunkSize));
		}
	}

	client->SetChunkBlocks(chunk->GetPosition(), client_set_blocks);

	auto current_tick = p_task_pool->GetWorld().GetCurrentTick();
	for (uint32_t i = 0; i < 27; ++i) {
		ChunkPos3 nei_pos;
		Chunk::NeighbourIndex2CmpXYZ(i, glm::value_ptr(nei_pos));
		nei_pos += chunk->GetPosition();
		// Trigger remesh
		if (neighbour_remesh_set[i])
			p_task_pool->Push<ChunkTaskType::kMesh>(nei_pos, true);
		// Trigger block update
		if (!block_updates[i].empty() || !block_activates[i].empty()) {
			p_task_pool->Push<ChunkTaskType::kUpdateBlock>(
			    nei_pos, std::vector<InnerIndex3>(block_updates[i].begin(), block_updates[i].end()), current_tick,
			    std::vector<InnerIndex3>(block_activates[i].begin(), block_activates[i].end()));
		}
	}

	// Trigger sunlight flood
	auto flood_sunlight_vec = std::vector<InnerIndex2>{flood_sunlights.begin(), flood_sunlights.end()};
	p_task_pool->Push<ChunkTaskType::kFloodSunlight>(chunk->GetPosition(), std::span{flood_sunlight_vec});
}

} // namespace hc::client