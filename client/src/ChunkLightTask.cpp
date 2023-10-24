#include <client/ChunkTaskPool.hpp>

#include <client/World.hpp>

namespace hc::client {

std::optional<ChunkTaskRunnerData<ChunkTaskType::kLight>>
ChunkTaskData<ChunkTaskType::kLight>::Pop(const ChunkTaskPoolLocked &task_pool, const ChunkPos3 &chunk_pos) {
	if (m_light_updates.empty())
		return std::nullopt;

	std::array<std::shared_ptr<Chunk>, 27> chunks;
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

	auto light_updates = std::move(m_light_updates);
	m_light_updates.clear();

	return ChunkTaskRunnerData<ChunkTaskType::kLight>{std::move(chunks), std::move(light_updates)};
}

void ChunkTaskRunner<ChunkTaskType::kLight>::Run(ChunkTaskPool *p_task_pool,
                                                 ChunkTaskRunnerData<ChunkTaskType::kLight> &&data) {
	const auto &neighbour_chunks = data.GetChunkPtrArray();
	const auto &chunk = neighbour_chunks.back();

	const auto get_block_func = [&neighbour_chunks](auto x, auto y, auto z) -> block::Block {
		return neighbour_chunks[Chunk::GetBlockNeighbourIndex(x, y, z)]->GetBlockFromNeighbour(x, y, z);
	};
}

} // namespace hc::client
