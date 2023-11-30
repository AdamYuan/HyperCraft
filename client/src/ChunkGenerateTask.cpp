#include <client/ChunkTaskPool.hpp>
#include <client/ClientBase.hpp>

#include <glm/gtx/string_cast.hpp>

namespace hc::client {

std::optional<ChunkTaskRunnerData<ChunkTaskType::kGenerate>>
ChunkTaskData<ChunkTaskType::kGenerate>::Pop(const ChunkTaskPoolLocked &task_pool, const ChunkPos3 &chunk_pos) {
	if (!m_queued)
		return std::nullopt;
	m_queued = false;

	std::shared_ptr<Chunk> chunk = task_pool.GetWorld().GetChunkPool().FindRawChunk(chunk_pos);
	if (!chunk || chunk->IsGenerated())
		return std::nullopt;

	return ChunkTaskRunnerData<ChunkTaskType::kGenerate>{std::move(chunk), std::move(m_chunk_entry)};
}

void ChunkTaskRunner<ChunkTaskType::kGenerate>::Run(ChunkTaskPool *p_task_pool,
                                                    ChunkTaskRunnerData<ChunkTaskType::kGenerate> &&data) {
	std::shared_ptr<ClientBase> client = p_task_pool->GetWorld().LockClient();
	if (!client)
		return;

	const auto &chunk_ptr = data.GetChunkPtr();

	client->GetTerrain()->Generate(chunk_ptr);
	// apply block updates
	for (const auto &block_entry : data.GetChunkEntry().blocks)
		chunk_ptr->SetBlock(block_entry.GetIndex(), block_entry.GetBlock());
	for (const auto &sunlight_entry : data.GetChunkEntry().sunlights)
		chunk_ptr->SetSunlightHeight(sunlight_entry.GetIndex(), sunlight_entry.GetSunlight());

	chunk_ptr->SetGeneratedFlag();

	p_task_pool->Push<ChunkTaskType::kMesh>(data.GetChunkPos());
}

} // namespace hc::client