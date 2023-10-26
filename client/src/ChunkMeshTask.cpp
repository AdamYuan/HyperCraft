#include <client/ChunkTaskPool.hpp>

#include <client/BlockLightAlgo.hpp>
#include <client/BlockMeshAlgo.hpp>
#include <client/World.hpp>
#include <client/WorldRenderer.hpp>

namespace hc::client {

template <std::signed_integral T> static inline constexpr uint32_t chunk_xyz_extended15_to_index(T x, T y, T z) {
	bool x_inside = 0 <= x && x < kChunkSize, y_inside = 0 <= y && y < kChunkSize, z_inside = 0 <= z && z < kChunkSize;
	uint32_t bits = x_inside | (y_inside << 1u) | (z_inside << 2u);
	if (bits == 7u)
		return ChunkXYZ2Index(x, y, z);
	constexpr uint32_t kOffsets[8] = {
	    kChunkSize * kChunkSize * kChunkSize,
	    kChunkSize * kChunkSize * kChunkSize + 30 * 30 * 30,
	    kChunkSize * kChunkSize * kChunkSize + 30 * 30 * 30 + 30 * kChunkSize * 30,
	    kChunkSize * kChunkSize * kChunkSize + 30 * 30 * 30 + 30 * kChunkSize * 30 * 2,
	    kChunkSize * kChunkSize * kChunkSize + 30 * 30 * 30 + 30 * kChunkSize * 30 * 2 + kChunkSize * kChunkSize * 30,
	    kChunkSize * kChunkSize * kChunkSize + 30 * 30 * 30 + 30 * kChunkSize * 30 * 3 + kChunkSize * kChunkSize * 30,
	    kChunkSize * kChunkSize * kChunkSize + 30 * 30 * 30 + 30 * kChunkSize * 30 * 3 +
	        kChunkSize * kChunkSize * 30 * 2,
	    0};
	constexpr uint32_t kMultipliers[8][3] = {{30, 30, 30},
	                                         {kChunkSize, 30, 30},
	                                         {30, kChunkSize, 30},
	                                         {kChunkSize, kChunkSize, 30},
	                                         {30, 30, kChunkSize},
	                                         {kChunkSize, 30, kChunkSize},
	                                         {30, kChunkSize, kChunkSize},
	                                         {kChunkSize, kChunkSize, kChunkSize}};
	if (!x_inside)
		x = x < 0 ? x + 15 : x - (int32_t)kChunkSize + 15;
	if (!y_inside)
		y = y < 0 ? y + 15 : y - (int32_t)kChunkSize + 15;
	if (!z_inside)
		z = z < 0 ? z + 15 : z - (int32_t)kChunkSize + 15;
	return kOffsets[bits] + kMultipliers[bits][0] * (kMultipliers[bits][2] * y + z) + x;
}

std::optional<ChunkTaskRunnerData<ChunkTaskType::kMesh>>
ChunkTaskData<ChunkTaskType::kMesh>::Pop(const ChunkTaskPoolLocked &task_pool, const ChunkPos3 &chunk_pos) {
	if (!m_queued)
		return std::nullopt;

	std::array<std::shared_ptr<Chunk>, 27> chunks;

	for (uint32_t i = 0; i < 27; ++i) {
		ChunkPos3 nei_pos;
		Chunk::NeighbourIndex2CmpXYZ(i, glm::value_ptr(nei_pos));
		nei_pos += chunk_pos;

		if (task_pool.AnyNotIdle<ChunkTaskType::kGenerate, ChunkTaskType::kSetBlock, ChunkTaskType::kSunlight>(nei_pos))
			return std::nullopt;

		std::shared_ptr<Chunk> nei_chunk = task_pool.GetWorld().GetChunkPool().FindChunk(nei_pos);
		if (nei_chunk == nullptr)
			return std::nullopt;
		chunks[i] = std::move(nei_chunk);
	}

	m_queued = false;
	return ChunkTaskRunnerData<ChunkTaskType::kMesh>{std::move(chunks)};
}

void ChunkTaskRunner<ChunkTaskType::kMesh>::Run(ChunkTaskPool *p_task_pool,
                                                ChunkTaskRunnerData<ChunkTaskType::kMesh> &&data) {
	const auto &neighbour_chunks = data.GetChunkPtrArray();
	const auto &chunk = neighbour_chunks.back();

	std::vector<BlockMesh> meshes;

	// Always recalculate lighting
	for (InnerPos1 y = -15; y < (InnerPos1)kChunkSize + 15; ++y)
		for (InnerPos1 z = -15; z < (InnerPos1)kChunkSize + 15; ++z)
			for (InnerPos1 x = -15; x < (InnerPos1)kChunkSize + 15; ++x) {
				block::Light light = {};
				if (IsValidChunkPosition(x, y, z)) {
					light.SetTorchlight(chunk->GetBlock(x, y, z).GetLightLevel());
					light.SetSunlight(chunk->GetSunlight(x, y, z) ? 15 : 0);
				} else {
					uint32_t nei_idx = Chunk::GetBlockNeighbourIndex(x, y, z);
					light.SetTorchlight(neighbour_chunks[nei_idx]->GetBlockFromNeighbour(x, y, z).GetLightLevel());
					light.SetSunlight(neighbour_chunks[nei_idx]->GetSunlightFromNeighbour(x, y, z) ? 15 : 0);
				}
				m_extend_light_buffer[chunk_xyz_extended15_to_index(x, y, z)] = light;
				if (LightAlgo::IsBorderLightInterfere(x, y, z, light.GetSunlight()))
					m_sunlight_entries.push({{x, y, z}, light.GetSunlight()});
				if (LightAlgo::IsBorderLightInterfere(x, y, z, light.GetTorchlight()))
					m_torchlight_entries.push({{x, y, z}, light.GetTorchlight()});
			}

	LightAlgo algo{};
	algo.PropagateLight(
	    &m_sunlight_entries,
	    [&neighbour_chunks](auto x, auto y, auto z) -> block::Block {
		    return neighbour_chunks[Chunk::GetBlockNeighbourIndex(x, y, z)]->GetBlockFromNeighbour(x, y, z);
	    },
	    [this](auto x, auto y, auto z) -> block::LightLvl {
		    return m_extend_light_buffer[chunk_xyz_extended15_to_index(x, y, z)].GetSunlight();
	    },
	    [this](auto x, auto y, auto z, block::LightLvl lvl) {
		    m_extend_light_buffer[chunk_xyz_extended15_to_index(x, y, z)].SetSunlight(lvl);
	    });
	algo.PropagateLight(
	    &m_torchlight_entries,
	    [&neighbour_chunks](auto x, auto y, auto z) -> block::Block {
		    return neighbour_chunks[Chunk::GetBlockNeighbourIndex(x, y, z)]->GetBlockFromNeighbour(x, y, z);
	    },
	    [this](auto x, auto y, auto z) -> block::LightLvl {
		    return m_extend_light_buffer[chunk_xyz_extended15_to_index(x, y, z)].GetTorchlight();
	    },
	    [this](auto x, auto y, auto z, block::LightLvl lvl) {
		    m_extend_light_buffer[chunk_xyz_extended15_to_index(x, y, z)].SetTorchlight(lvl);
	    });

	meshes =
	    BlockMeshAlgo<BlockAlgoConfig<uint32_t, BlockAlgoBound<uint32_t>{0, 0, 0, kChunkSize, kChunkSize, kChunkSize},
	                                  kBlockAlgoSwizzleYZX>>{}
	        .Generate(
	            [&neighbour_chunks](auto x, auto y, auto z) -> block::Block {
		            return neighbour_chunks[Chunk::GetBlockNeighbourIndex(x, y, z)]->GetBlockFromNeighbour(x, y, z);
	            },
	            [this](auto x, auto y, auto z) -> block::Light {
		            return m_extend_light_buffer[chunk_xyz_extended15_to_index(x, y, z)];
	            });

	auto renderer = p_task_pool->GetWorld().LockRenderer();
	if (renderer)
		renderer->PushChunkMesh(chunk->GetPosition(), std::move(meshes));
}

} // namespace hc::client