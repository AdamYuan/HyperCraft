#include <client/ChunkTaskPool.hpp>
#include <client/ClientBase.hpp>

#include <glm/gtx/string_cast.hpp>

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

	client->GetTerrain()->Generate(chunk_ptr);
	// apply block updates
	p_task_pool->GetWorld().m_chunk_update_pool.ApplyBlockUpdates(chunk_ptr);
	// apply sunlight updates
	p_task_pool->GetWorld().m_chunk_update_pool.ApplySunlightUpdates(chunk_ptr);

	chunk_ptr->SetGeneratedFlag();

	p_task_pool->Push<ChunkTaskType::kMesh>(data.GetChunkPos());
}

} // namespace hc::client