#include <client/ChunkTaskPool.hpp>
#include <client/ClientBase.hpp>

#include <glm/gtx/string_cast.hpp>

namespace hc::client {

std::optional<ChunkTaskRunnerData<ChunkTaskType::kGenerate>>
ChunkTaskData<ChunkTaskType::kGenerate>::Pop(const ChunkTaskPoolLocked &task_pool, const ChunkPos3 &chunk_pos) {
	if (!m_queued)
		return std::nullopt;
	std::shared_ptr<Chunk> chunk = task_pool.GetWorld().GetChunkPool().FindChunk(chunk_pos);
	if (!chunk)
		return std::nullopt;
	m_queued = false;
	return ChunkTaskRunnerData<ChunkTaskType::kGenerate>{std::move(chunk)};
}

void ChunkTaskRunner<ChunkTaskType::kGenerate>::Run(ChunkTaskPool *p_task_pool,
                                                    ChunkTaskRunnerData<ChunkTaskType::kGenerate> &&data) {
	std::shared_ptr<ClientBase> client = p_task_pool->GetWorld().LockClient();
	if (!client || !client->IsConnected())
		return;

	const auto &chunk_ptr = data.GetChunkPtr();

	client->GetTerrain()->Generate(chunk_ptr, m_light_map);
	auto updates = p_task_pool->GetWorld().GetChunkUpdatePool().GetBlockUpdate(chunk_ptr->GetPosition());
	for (const auto &u : updates)
		chunk_ptr->SetBlock(u.first.x, u.first.y, u.first.z, u.second);

	// set initial sunlight from light_map
	BlockPos1 base_height = data.GetChunkPos().y * (BlockPos1)kChunkSize;
	for (uint32_t idx = 0; idx < kChunkSize * kChunkSize; ++idx)
		chunk_ptr->SetSunlightHeight(
		    idx, (InnerPos1)(std::clamp(m_light_map[idx] - base_height + 1, 0, (BlockPos1)kChunkSize)));

	// printf("Generate %s\n", glm::to_string(data.GetChunkPos()).c_str());

	p_task_pool->Push<ChunkTaskType::kMesh>(data.GetChunkPos());
}

} // namespace hc::client