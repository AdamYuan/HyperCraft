#include <client/ChunkUpdatePool.hpp>

#include <client/World.hpp>

namespace hc::client {

void ChunkUpdatePool::SetBlockUpdate(ChunkPos3 chunk_pos, InnerPos3 inner_pos, block::Block block) {
	m_block_updates.uprase_fn(
	    chunk_pos,
	    [this, &chunk_pos, inner_pos, block](auto &data, libcuckoo::UpsertContext) {
		    data[inner_pos] = block;
		    m_world.m_chunk_task_pool.Push<ChunkTaskType::kSetBlock, ChunkTaskPriority::kHigh>(chunk_pos, inner_pos,
		                                                                                       block);
		    return false;
	    },
	    InnerPosCompare{});
}

void ChunkUpdatePool::SetBlockUpdate(ChunkPos3 chunk_pos, std::span<std::pair<InnerPos3, block::Block>> blocks) {
	m_block_updates.uprase_fn(
	    chunk_pos,
	    [this, &chunk_pos, blocks](auto &data, libcuckoo::UpsertContext) {
		    for (const auto &i : blocks)
			    data[i.first] = i.second;
		    m_world.m_chunk_task_pool.Push<ChunkTaskType::kSetBlock, ChunkTaskPriority::kHigh>(chunk_pos, blocks);
		    return false;
	    },
	    InnerPosCompare{});
}

void ChunkUpdatePool::SetSunlightUpdate(ChunkPos3 chunk_pos, InnerPos2 inner_pos, InnerPos1 sunlight_height) {
	m_sunlight_updates.uprase_fn(
	    chunk_pos,
	    [this, &chunk_pos, inner_pos, sunlight_height](auto &data, libcuckoo::UpsertContext) {
		    data[inner_pos] = sunlight_height;
		    m_world.m_chunk_task_pool.Push<ChunkTaskType::kSetSunlight, ChunkTaskPriority::kHigh>(chunk_pos, inner_pos,
		                                                                                          sunlight_height);
		    return false;
	    },
	    InnerPosCompare{});
}
void ChunkUpdatePool::SetSunlightUpdate(ChunkPos3 chunk_pos, std::span<std::pair<InnerPos2, InnerPos1>> sunlights) {
	m_sunlight_updates.uprase_fn(
	    chunk_pos,
	    [this, &chunk_pos, sunlights](auto &data, libcuckoo::UpsertContext) {
		    for (const auto &i : sunlights)
			    data[i.first] = i.second;
		    m_world.m_chunk_task_pool.Push<ChunkTaskType::kSetSunlight, ChunkTaskPriority::kHigh>(chunk_pos, sunlights);
		    return false;
	    },
	    InnerPosCompare{});
}

} // namespace hc::client