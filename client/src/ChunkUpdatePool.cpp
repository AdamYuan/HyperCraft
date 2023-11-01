#include <client/ChunkUpdatePool.hpp>

#include <client/World.hpp>

namespace hc::client {

void ChunkUpdatePool::SetBlockUpdate(ChunkPos3 chunk_pos, InnerPos3 inner_pos, block::Block block, bool active) {
	m_block_updates.uprase_fn(
	    chunk_pos,
	    [this, &chunk_pos, inner_pos, block, active](auto &data, libcuckoo::UpsertContext) {
		    data[inner_pos] = block;
		    m_world.m_chunk_task_pool.Push<ChunkTaskType::kSetBlock>(chunk_pos, inner_pos, block, active);
		    return false;
	    },
	    InnerPosCompare{});
}

void ChunkUpdatePool::SetBlockUpdateBulk(ChunkPos3 chunk_pos,
                                         std::span<const std::pair<InnerPos3, block::Block>> blocks, bool active) {
	m_block_updates.uprase_fn(
	    chunk_pos,
	    [this, &chunk_pos, blocks, active](auto &data, libcuckoo::UpsertContext) {
		    for (const auto &i : blocks)
			    data[i.first] = i.second;
		    m_world.m_chunk_task_pool.Push<ChunkTaskType::kSetBlock>(chunk_pos, blocks, active);
		    return false;
	    },
	    InnerPosCompare{});
}

void ChunkUpdatePool::SetSunlightUpdate(ChunkPos3 chunk_pos, InnerPos2 inner_pos, InnerPos1 sunlight_height,
                                        bool active) {
	m_sunlight_updates.uprase_fn(
	    chunk_pos,
	    [this, &chunk_pos, inner_pos, sunlight_height, active](auto &data, libcuckoo::UpsertContext) {
		    data[inner_pos] = sunlight_height;
		    m_world.m_chunk_task_pool.Push<ChunkTaskType::kSetSunlight>(chunk_pos, inner_pos, sunlight_height, active);
		    return false;
	    },
	    InnerPosCompare{});
}
void ChunkUpdatePool::SetSunlightUpdateBulk(ChunkPos3 chunk_pos,
                                            std::span<const std::pair<InnerPos2, InnerPos1>> sunlights, bool active) {
	m_sunlight_updates.uprase_fn(
	    chunk_pos,
	    [this, &chunk_pos, sunlights, active](auto &data, libcuckoo::UpsertContext) {
		    for (const auto &i : sunlights)
			    data[i.first] = i.second;
		    m_world.m_chunk_task_pool.Push<ChunkTaskType::kSetSunlight>(chunk_pos, sunlights, active);
		    return false;
	    },
	    InnerPosCompare{});
}

} // namespace hc::client