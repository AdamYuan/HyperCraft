#include <client/ChunkTaskPool.hpp>

#include <client/World.hpp>

namespace hc::client {

std::optional<ChunkTaskRunnerData<ChunkTaskType::kUpdateBlock>>
ChunkTaskData<ChunkTaskType::kUpdateBlock>::Pop(const ChunkTaskPoolLocked &task_pool, const ChunkPos3 &chunk_pos) {
	if (m_updates.empty())
		return std::nullopt;

	std::array<std::shared_ptr<Chunk>, 27> chunks;
	auto &chunk = chunks[26];

	for (uint32_t i = 0; i < 27; ++i) {
		ChunkPos3 nei_pos;
		Chunk::NeighbourIndex2CmpXYZ(i, glm::value_ptr(nei_pos));
		nei_pos += chunk_pos;

		if (task_pool.AnyNotIdle<ChunkTaskType::kGenerate, ChunkTaskType::kSetBlock>(nei_pos))
			return std::nullopt;

		std::shared_ptr<Chunk> nei_chunk = task_pool.GetWorld().GetChunkPool().FindChunk(nei_pos);
		if (nei_chunk == nullptr)
			return std::nullopt;
		chunks[i] = std::move(nei_chunk);
	}

	std::vector<InnerPos3> updates_to_apply;

	uint64_t current_tick = task_pool.GetWorld().GetCurrentTick();
	for (auto it = m_updates.begin(); it != m_updates.end();) {
		auto pos = it->first;
		auto block = chunk->GetBlock(pos.x, pos.y, pos.z);

		if (block.GetEvent() == nullptr || block.GetEvent()->on_update_func == nullptr)
			it = m_updates.erase(it);
		else if (it->second + block.GetEvent()->update_tick_interval <= current_tick) {
			updates_to_apply.push_back(pos);
			it = m_updates.erase(it);
		} else
			++it;
	}
	if (updates_to_apply.empty())
		return std::nullopt;

	return ChunkTaskRunnerData<ChunkTaskType::kUpdateBlock>{std::move(chunks), std::move(updates_to_apply)};
}

void ChunkTaskRunner<ChunkTaskType::kUpdateBlock>::Run(ChunkTaskPool *p_task_pool,
                                                       ChunkTaskRunnerData<ChunkTaskType::kUpdateBlock> &&data) {
	const auto &neighbour_chunks = data.GetChunkPtrArray();
	const auto &chunk = neighbour_chunks.back();

	const auto get_block = [&neighbour_chunks](auto x, auto y, auto z) -> block::Block {
		return neighbour_chunks[Chunk::GetBlockNeighbourIndex(x, y, z)]->GetBlockFromNeighbour(x, y, z);
	};

	std::vector<std::pair<InnerPos3, block::Block>> set_blocks[27];

	for (const auto &update : data.GetUpdates()) {
		const auto *p_blk_event = chunk->GetBlock(update.x, update.y, update.z).GetEvent();
		if (!p_blk_event || !p_blk_event->on_update_func)
			continue;
		for (uint32_t i = 0; i < p_blk_event->update_neighbour_count; ++i) {
			InnerPos3 pos = update + p_blk_event->update_neighbours[i];
			m_update_neighbour_pos[i] = pos;
			m_update_neighbours[i] = get_block(pos.x, pos.y, pos.z);
		}
		uint32_t set_block_count = 0;
		p_blk_event->on_update_func(m_update_neighbours, m_update_set_blocks, &set_block_count);
		for (uint32_t i = 0; i < set_block_count; ++i) {
			InnerPos3 set_pos = m_update_neighbour_pos[m_update_set_blocks[i].idx];
			block::Block set_blk = m_update_set_blocks[i].block;
			auto [rel_chunk_pos, inner_pos] = ChunkInnerPosFromBlockPos(BlockPos3(set_pos));
			uint32_t nei_chunk_idx = CmpXYZ2NeighbourIndex(rel_chunk_pos.x, rel_chunk_pos.y, rel_chunk_pos.z);
			set_blocks[nei_chunk_idx].emplace_back(inner_pos, set_blk);
		}
	}

	for (uint32_t i = 0; i < 27; ++i) {
		if (set_blocks[i].empty())
			continue;
		ChunkPos3 nei_pos;
		Chunk::NeighbourIndex2CmpXYZ(i, glm::value_ptr(nei_pos));
		nei_pos += chunk->GetPosition();
		p_task_pool->GetWorld().m_chunk_update_pool.SetBlockUpdateBulk(nei_pos, set_blocks[i], true);
	}
}

} // namespace hc::client