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

void ChunkTaskRunner<ChunkTaskType::kSetSunlight>::RunWithoutData(
    ChunkTaskPool *p_task_pool, const LockedChunk<ChunkLockType::kSunlightRW> &locked_chunk,
    std::span<const std::pair<InnerPos2, InnerPos1>> sunlights, bool active,
    const std::function<void()> &on_write_done_func) {
	std::unordered_map<InnerPos2, InnerPos1> sunlight_changes;
	for (const auto &set_sunlight : sunlights)
		sunlight_changes[set_sunlight.first] = set_sunlight.second;

	auto chunk_pos = locked_chunk.GetPosition();

	std::bitset<27> neighbour_remesh_set{};

	for (const auto &sunlight_change : sunlight_changes) {
		auto xz_pos = sunlight_change.first;
		auto xz_idx = ChunkXZ2Index(xz_pos.x, xz_pos.y);
		auto new_sunlight = sunlight_change.second, old_sunlight = locked_chunk.GetSunlightHeight(xz_idx);
		if (new_sunlight == old_sunlight)
			continue;

		neighbour_remesh_set[26] = true;
		locked_chunk.SetSunlightHeight(xz_idx, new_sunlight);

		for (uint32_t i = 0; i < 26; ++i) {
			if (neighbour_remesh_set[i])
				continue;
			ChunkPos3 nei_chunk_pos;
			NeighbourIndex2CmpXYZ(i, glm::value_ptr(nei_chunk_pos));
			neighbour_remesh_set[i] = ChunkShouldRemesh({xz_pos.x, std::min(old_sunlight, new_sunlight), xz_pos.y}) ||
			                          ChunkShouldRemesh({xz_pos.x, std::max(old_sunlight, new_sunlight) - 1, xz_pos.y});
		}
	}

	// on_write_done_func();

	for (uint32_t i = 0; i < 27; ++i) {
		if (!neighbour_remesh_set[i])
			continue;
		ChunkPos3 nei_pos;
		NeighbourIndex2CmpXYZ(i, glm::value_ptr(nei_pos));
		nei_pos += chunk_pos;
		p_task_pool->Push<ChunkTaskType::kMesh>(nei_pos, active);
	}
}

void ChunkTaskRunner<ChunkTaskType::kSetSunlight>::Run(ChunkTaskPool *p_task_pool,
                                                       ChunkTaskRunnerData<ChunkTaskType::kSetSunlight> &&data) {
	const auto &chunk = data.GetChunkPtr();
	auto [locked_chunk] = Chunk::Lock<ChunkLockType::kSunlightRW>(chunk);
	RunWithoutData(p_task_pool, locked_chunk, data.GetSetSunlights(), data.IsHighPriority(),
	               [&locked_chunk] { locked_chunk.Unlock(); });
}

} // namespace hc::client
