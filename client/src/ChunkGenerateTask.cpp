#include <client/ChunkTaskPool.hpp>
#include <client/ClientBase.hpp>

#include <glm/gtx/string_cast.hpp>

namespace hc::client {

std::optional<ChunkTaskRunnerData<ChunkTaskType::kGenerate>>
ChunkTaskData<ChunkTaskType::kGenerate>::Pop(const ChunkTaskPool &task_pool, const ChunkPos3 &chunk_pos) {
	if (!m_queued)
		return std::nullopt;
	std::shared_ptr<Chunk> chunk;
	std::shared_ptr<ClientBase> client;
	if (!(chunk = task_pool.GetWorld().FindChunk(chunk_pos)) || !(client = task_pool.GetWorld().LockClient()))
		return std::nullopt;
	m_queued = false;
	return ChunkTaskRunnerData<ChunkTaskType::kGenerate>{std::move(chunk), std::move(client)};
}

void ChunkTaskRunner<ChunkTaskType::kGenerate>::Run(ChunkTaskPool *p_task_pool,
                                                    ChunkTaskRunnerData<ChunkTaskType::kGenerate> &&data) {
	// If client not connected, return
	if (!data.GetClientPtr()->IsConnected())
		return;

	const auto &chunk_ptr = data.GetChunkPtr();

	data.GetClientPtr()->GetTerrain()->Generate(chunk_ptr, m_light_map);
	// set initial sunlight from light_map
	for (uint32_t y = 0; y < Chunk::kSize; ++y) {
		int32_t cur_height = data.GetChunkPos().y * (int)Chunk::kSize + (int)y;
		for (uint32_t idx = 0; idx < kChunkSize * kChunkSize; ++idx) {
			if (cur_height > m_light_map[idx])
				chunk_ptr->SetLight(y * kChunkSize * kChunkSize + idx, {15, 0});
		}
	}

	// printf("Generate %s\n", glm::to_string(data.GetChunkPos()).c_str());

	p_task_pool->Push<ChunkTaskType::kMesh>(data.GetChunkPos(), true);
}

} // namespace hc::client