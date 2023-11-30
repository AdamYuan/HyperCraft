#include <client/ChunkTaskPool.hpp>

#include <client/ClientBase.hpp>
#include <client/World.hpp>

namespace hc::client {

std::optional<ChunkTaskRunnerData<ChunkTaskType::kSetSunlight>>
ChunkTaskData<ChunkTaskType::kSetSunlight>::Pop(const ChunkTaskPoolLocked &task_pool, const ChunkPos3 &chunk_pos) {
	if (m_set_sunlight_map.empty())
		return std::nullopt;

	if (task_pool.AnyNotIdle<ChunkTaskType::kGenerate>(chunk_pos))
		return std::nullopt;

	// if the bottom chunk is flooding, postpone
	if (task_pool.AnyRunning<ChunkTaskType::kFloodSunlight>({chunk_pos.x, chunk_pos.y - 1, chunk_pos.z}))
		return std::nullopt;

	std::shared_ptr<Chunk> chunk;
	if (!(chunk = task_pool.GetWorld().GetChunkPool().FindChunk(chunk_pos)))
		return std::nullopt;
	auto set_sunlight_map = std::move(m_set_sunlight_map);
	m_set_sunlight_map.clear();
	return ChunkTaskRunnerData<ChunkTaskType::kSetSunlight>{std::move(chunk), std::move(set_sunlight_map)};
}

void ChunkTaskRunner<ChunkTaskType::kSetSunlight>::Run(ChunkTaskPool *p_task_pool,
                                                       ChunkTaskRunnerData<ChunkTaskType::kSetSunlight> &&data) {
	std::shared_ptr<ClientBase> client = p_task_pool->GetWorld().LockClient();
	if (!client)
		return;

	const auto &chunk = data.GetChunkPtr();

	std::bitset<27> neighbour_remesh_set{};

	std::vector<InnerIndex2> xz_updates;
	std::vector<ChunkSetSunlightEntry> client_set_sunlights;

	for (const auto &it : data.GetSetSunlightMap()) {
		const auto &update = it.second;

		auto xz_idx = it.first;
		auto xz_pos = InnerPos2FromIndex(xz_idx);
		auto new_sunlight = update.GetNew(), local_old_sunlight = chunk->GetSunlightHeight(xz_idx);
		auto opt_old_sunlight = update.GetOld();

		if (update.Get<ChunkUpdateType::kLocal>().has_value())
			client_set_sunlights.push_back(
			    {.index = xz_idx,
			     .old_sunlight = opt_old_sunlight.has_value() ? opt_old_sunlight.value() : local_old_sunlight,
			     .new_sunlight = new_sunlight});

		if (new_sunlight == local_old_sunlight)
			continue;

		chunk->SetSunlightHeight(xz_idx, new_sunlight);

		// activate flood
		xz_updates.push_back(xz_idx);

		// update remesh set
		neighbour_remesh_set[26] = true;
		for (uint32_t i = 0; i < 26; ++i) {
			if (neighbour_remesh_set[i])
				continue;
			ChunkPos3 nei_chunk_pos;
			Chunk::NeighbourIndex2CmpXYZ(i, glm::value_ptr(nei_chunk_pos));
			neighbour_remesh_set[i] =
			    ChunkShouldRemesh({xz_pos.x, std::min(local_old_sunlight, new_sunlight), xz_pos.y}) ||
			    ChunkShouldRemesh({xz_pos.x, std::max(local_old_sunlight, new_sunlight) - 1, xz_pos.y});
		}
	}

	client->SetChunkSunlights(chunk->GetPosition(), client_set_sunlights);

	for (uint32_t i = 0; i < 27; ++i) {
		if (!neighbour_remesh_set[i])
			continue;
		ChunkPos3 nei_pos;
		Chunk::NeighbourIndex2CmpXYZ(i, glm::value_ptr(nei_pos));
		nei_pos += chunk->GetPosition();
		p_task_pool->Push<ChunkTaskType::kMesh>(nei_pos, false); // not high priority
	}

	auto down_chunk_pos = chunk->GetPosition();
	--down_chunk_pos.y;
	p_task_pool->Push<ChunkTaskType::kFloodSunlight>(down_chunk_pos, xz_updates);
}

} // namespace hc::client
