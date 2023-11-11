#include <client/ChunkTaskPool.hpp>
#include <client/ClientBase.hpp>

namespace hc::client {

std::optional<ChunkTaskRunnerData<ChunkTaskType::kGenerate>>
ChunkTaskData<ChunkTaskType::kGenerate>::Pop(const ChunkTaskPoolLocked &task_pool, const ChunkPos3 &chunk_pos) {
	if (!m_queued)
		return std::nullopt;
	std::shared_ptr<Chunk> chunk = task_pool.GetWorld().GetChunkPool().FindRawChunk(chunk_pos);
	if (!chunk)
		return std::nullopt;
	m_queued = false;
	return ChunkTaskRunnerData<ChunkTaskType::kGenerate>{std::move(chunk)};
}

void ChunkTaskRunner<ChunkTaskType::kGenerate>::Run(ChunkTaskPool *p_task_pool,
                                                    ChunkTaskRunnerData<ChunkTaskType::kGenerate> &&data) {
	std::shared_ptr<ClientBase> client = p_task_pool->GetWorld().LockClient();
	if (!client)
		return;

	const auto &chunk_ptr = data.GetChunkPtr();

	{
		auto [locked_chunk] = Chunk::Lock<ChunkLockType::kAllRW>(chunk_ptr);

		client->GetTerrain()->Generate(locked_chunk.As<ChunkLockType::kBlockRW>(), m_y_peak_map);
		// apply block updates
		auto block_updates = p_task_pool->GetWorld().GetChunkUpdatePool().GetBlockUpdateBulk(chunk_ptr->GetPosition());
		for (const auto &u : block_updates)
			locked_chunk.SetBlock(u.first.x, u.first.y, u.first.z, u.second);

		// set initial sunlight from light_map
		BlockPos1 base_height = data.GetChunkPos().y * (BlockPos1)kChunkSize;
		for (uint32_t idx = 0; idx < kChunkSize * kChunkSize; ++idx)
			locked_chunk.SetSunlightHeight(
			    idx, (InnerPos1)(std::clamp(m_y_peak_map[idx] - base_height + 1, 0, (BlockPos1)kChunkSize)));
		// apply sunlight updates
		auto sunlight_updates =
		    p_task_pool->GetWorld().GetChunkUpdatePool().GetSunlightUpdateBulk(chunk_ptr->GetPosition());
		for (const auto &u : sunlight_updates)
			locked_chunk.SetSunlightHeight(u.first.x, u.first.y, u.second);
	}
	chunk_ptr->SetGeneratedFlag();

	p_task_pool->Push<ChunkTaskType::kMesh>(data.GetChunkPos());
}

} // namespace hc::client