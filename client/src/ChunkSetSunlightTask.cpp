#include <client/ChunkTaskPool.hpp>

#include <client/World.hpp>

namespace hc::client {

std::optional<ChunkTaskRunnerData<ChunkTaskType::kSetSunlight>>
ChunkTaskData<ChunkTaskType::kSetSunlight>::Pop(const ChunkTaskPoolLocked &task_pool, const ChunkPos3 &chunk_pos) {
	if (m_set_sunlights.empty())
		return std::nullopt;

	if (task_pool.AnyNotIdle<ChunkTaskType::kGenerate>(chunk_pos))
		return std::nullopt;
	std::shared_ptr<Chunk> chunk;
	if (!(chunk = task_pool.GetWorld().GetChunkPool().FindChunk(chunk_pos)))
		return std::nullopt;
	auto set_sunlights = std::move(m_set_sunlights);
	m_set_sunlights.clear();
	bool high_priority = m_high_priority;
	m_high_priority = false;
	return ChunkTaskRunnerData<ChunkTaskType::kSetSunlight>{std::move(chunk), std::move(set_sunlights), high_priority};
}

void ChunkTaskRunner<ChunkTaskType::kSetSunlight>::Run(ChunkTaskPool *p_task_pool,
                                                       ChunkTaskRunnerData<ChunkTaskType::kSetSunlight> &&data) {
	std::unordered_map<InnerPos2, InnerPos1> sunlight_changes;
	for (const auto &set_sunlight : data.GetSetSunlights())
		sunlight_changes[set_sunlight.first] = set_sunlight.second;

	const auto &chunk = data.GetChunkPtr();

	std::bitset<27> neighbour_remesh_set{};

	for (const auto &sunlight_change : sunlight_changes) {
		auto xz_pos = sunlight_change.first;
		auto xz_idx = ChunkXZ2Index(xz_pos.x, xz_pos.y);
		auto new_sunlight = sunlight_change.second, old_sunlight = chunk->GetSunlightHeight(xz_idx);
		if (new_sunlight == old_sunlight)
			continue;

		neighbour_remesh_set[26] = true;
		chunk->SetSunlightHeight(xz_idx, new_sunlight);

		for (uint32_t i = 0; i < 26; ++i) {
			if (neighbour_remesh_set[i])
				continue;
			ChunkPos3 nei_chunk_pos;
			Chunk::NeighbourIndex2CmpXYZ(i, glm::value_ptr(nei_chunk_pos));
			neighbour_remesh_set[i] = ChunkShouldRemesh({xz_pos.x, std::min(old_sunlight, new_sunlight), xz_pos.y}) ||
			                          ChunkShouldRemesh({xz_pos.x, std::max(old_sunlight, new_sunlight) - 1, xz_pos.y});
		}
	}

	for (uint32_t i = 0; i < 27; ++i) {
		if (!neighbour_remesh_set[i])
			continue;
		ChunkPos3 nei_pos;
		Chunk::NeighbourIndex2CmpXYZ(i, glm::value_ptr(nei_pos));
		nei_pos += chunk->GetPosition();
		p_task_pool->Push<ChunkTaskType::kMesh>(nei_pos, data.IsHighPriority());
	}
}

} // namespace hc::client
