#include <client/ChunkTaskPool.hpp>

#include <client/World.hpp>

#include <glm/gtx/hash.hpp>

namespace hc::client {

std::optional<ChunkTaskRunnerData<ChunkTaskType::kFloodSunlight>>
ChunkTaskData<ChunkTaskType::kFloodSunlight>::Pop(const ChunkTaskPoolLocked &task_pool, const ChunkPos3 &chunk_pos) {
	if (m_xz_updates.empty())
		return std::nullopt;

	ChunkPos3 up_chunk_pos = {chunk_pos.x, chunk_pos.y + 1, chunk_pos.z};

	if (task_pool.AnyNotIdle<ChunkTaskType::kGenerate, ChunkTaskType::kSetBlock>(chunk_pos))
		return std::nullopt;
	if (task_pool.AnyNotIdle<ChunkTaskType::kGenerate, ChunkTaskType::kSetSunlight>(up_chunk_pos))
		return std::nullopt;
	std::shared_ptr<Chunk> chunk, up_chunk;
	if (!(chunk = task_pool.GetWorld().GetChunkPool().FindChunk(chunk_pos)))
		return std::nullopt;
	if (!(up_chunk = task_pool.GetWorld().GetChunkPool().FindChunk(up_chunk_pos)))
		return std::nullopt;

	auto xz_updates = std::move(m_xz_updates);
	m_xz_updates.clear();

	return ChunkTaskRunnerData<ChunkTaskType::kFloodSunlight>{std::move(chunk), std::move(up_chunk),
	                                                          std::move(xz_updates)};
}

void ChunkTaskRunner<ChunkTaskType::kFloodSunlight>::Run(ChunkTaskPool *p_task_pool,
                                                         ChunkTaskRunnerData<ChunkTaskType::kFloodSunlight> &&data) {
	std::unordered_set<InnerIndex2> xz_updates{data.GetXZUpdates().begin(), data.GetXZUpdates().end()};

	std::vector<ChunkSunlightEntry> set_sunlights;

	const auto &up_chunk = data.GetUpChunkPtr(), &chunk = data.GetChunkPtr();

	for (auto xz_idx : xz_updates) {
		auto up_sl = up_chunk->GetSunlightHeight(xz_idx); //, sl = chunk->GetSunlightHeight(xz_idx);
		if (up_sl > 0) {
			// if (sl != kChunkSize)
			set_sunlights.push_back({.index = xz_idx, .sunlight = kChunkSize});
		} else {
			InnerPos1 y;
			for (y = kChunkSize - 1;
			     (~y) && chunk->GetBlock(xz_idx + (uint32_t)y * kChunkSize * kChunkSize).GetVerticalLightPass(); --y)
				;
			++y;
			// if (sl != y)
			set_sunlights.push_back({.index = xz_idx, .sunlight = y});
			// if ((sl == 0) != (y == 0))
			// 	xz_next_updates.push_back(xz_idx);
		}
	}

	p_task_pool->Push<ChunkTaskType::kSetSunlight>(chunk->GetPosition(), set_sunlights, ChunkUpdateType::kLocal);
	// p_task_pool->GetWorld().m_chunk_update_pool.SetSunlightUpdateBulk(chunk->GetPosition(), set_sunlights,
	//                                                                   data.IsHighPriority());

	/* auto down_chunk_pos = chunk->GetPosition();
	--down_chunk_pos.y;
	p_task_pool->Push<ChunkTaskType::kFloodSunlight>(down_chunk_pos, xz_next_updates, false); */
}

} // namespace hc::client