#include <client/ChunkUpdatePool.hpp>

#include <client/World.hpp>

namespace hc::client {

void ChunkUpdatePool::SetBlockUpdate(ChunkPos3 chunk_pos, ChunkSetBlock set_block, bool active) {
	m_block_updates.uprase_fn(chunk_pos, [this, &chunk_pos, set_block, active](auto &data, libcuckoo::UpsertContext) {
		data[set_block.index] = set_block.block;
		m_world.m_chunk_task_pool.Push<ChunkTaskType::kSetBlock>(chunk_pos, set_block, active);
		return false;
	});
}

void ChunkUpdatePool::SetBlockUpdateBulk(ChunkPos3 chunk_pos, std::span<const ChunkSetBlock> set_blocks, bool active) {
	m_block_updates.uprase_fn(chunk_pos, [this, &chunk_pos, set_blocks, active](auto &data, libcuckoo::UpsertContext) {
		for (const auto &i : set_blocks)
			data[i.index] = i.block;
		m_world.m_chunk_task_pool.Push<ChunkTaskType::kSetBlock>(chunk_pos, set_blocks, active);
		return false;
	});
}

void ChunkUpdatePool::SetSunlightUpdate(ChunkPos3 chunk_pos, ChunkSetSunlight set_sunlight, bool active) {
	m_sunlight_updates.uprase_fn(
	    chunk_pos, [this, &chunk_pos, set_sunlight, active](auto &data, libcuckoo::UpsertContext) {
		    data[set_sunlight.index] = set_sunlight.sunlight;
		    m_world.m_chunk_task_pool.Push<ChunkTaskType::kSetSunlight>(chunk_pos, set_sunlight, active);
		    return false;
	    });
}
void ChunkUpdatePool::SetSunlightUpdateBulk(ChunkPos3 chunk_pos, std::span<const ChunkSetSunlight> set_sunlights,
                                            bool active) {
	m_sunlight_updates.uprase_fn(
	    chunk_pos, [this, &chunk_pos, set_sunlights, active](auto &data, libcuckoo::UpsertContext) {
		    for (const auto &i : set_sunlights)
			    data[i.index] = i.sunlight;
		    m_world.m_chunk_task_pool.Push<ChunkTaskType::kSetSunlight>(chunk_pos, set_sunlights, active);
		    return false;
	    });
}

} // namespace hc::client