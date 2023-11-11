#include <client/ChunkTaskPool.hpp>

#include <client/BlockLightAlgo.hpp>
#include <client/BlockMeshAlgo.hpp>
#include <client/World.hpp>
#include <client/WorldRenderer.hpp>

namespace hc::client {

template <std::signed_integral T> static inline constexpr uint32_t chunk_xyz_extended15_to_index(T x, T y, T z) {
	return x + 15 + ((z + 15) + (y + 15) * (kChunkSize + 30)) * (kChunkSize + 30);
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

		if (task_pool.AnyNotIdle<ChunkTaskType::kGenerate, ChunkTaskType::kSetBlock, ChunkTaskType::kSetSunlight>(
		        nei_pos))
			return std::nullopt;

		std::shared_ptr<Chunk> nei_chunk = task_pool.GetWorld().GetChunkPool().FindChunk(nei_pos);
		if (nei_chunk == nullptr)
			return std::nullopt;
		chunks[i] = std::move(nei_chunk);
	}

	m_queued = false;
	m_high_priority = false;
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
				uint32_t nei_idx = Chunk::GetBlockNeighbourIndex(x, y, z);
				light.SetTorchlight(neighbour_chunks[nei_idx]->GetBlockFromNeighbour(x, y, z).GetLightLevel());
				light.SetSunlight(neighbour_chunks[nei_idx]->GetSunlightFromNeighbour(x, y, z) ? 15 : 0);
				m_extend_light_buffer[chunk_xyz_extended15_to_index(x, y, z)] = light;
				if (LightAlgo::IsBorderLightInterfere(x, y, z, light.GetTorchlight()))
					m_torchlight_entries.push({{x, y, z}, light.GetTorchlight()});
			}

	for (InnerPos1 y = -15; y < (InnerPos1)kChunkSize + 15; ++y)
		for (InnerPos1 z = -15; z < (InnerPos1)kChunkSize + 15; ++z)
			for (InnerPos1 x = -15; x < (InnerPos1)kChunkSize + 15; ++x) {
				auto sunlight = m_extend_light_buffer[chunk_xyz_extended15_to_index(x, y, z)].GetSunlight();
				if (LightAlgo::IsBorderLightInterfere(x, y, z, sunlight)) {
					if ((x == -15 ||
					     m_extend_light_buffer[chunk_xyz_extended15_to_index(InnerPos1(x - 1), y, z)].GetSunlight() ==
					         15) &&
					    (x == (InnerPos1)kChunkSize + 14 ||
					     m_extend_light_buffer[chunk_xyz_extended15_to_index(InnerPos1(x + 1), y, z)].GetSunlight() ==
					         15) &&
					    (z == -15 ||
					     m_extend_light_buffer[chunk_xyz_extended15_to_index(x, y, InnerPos1(z - 1))].GetSunlight() ==
					         15) &&
					    (z == (InnerPos1)kChunkSize + 14 ||
					     m_extend_light_buffer[chunk_xyz_extended15_to_index(x, y, InnerPos1(z + 1))].GetSunlight() ==
					         15) &&
					    (y == -15 ||
					     m_extend_light_buffer[chunk_xyz_extended15_to_index(x, InnerPos1(y - 1), z)].GetSunlight() ==
					         15) &&
					    (y == (InnerPos1)kChunkSize + 14 ||
					     m_extend_light_buffer[chunk_xyz_extended15_to_index(x, InnerPos1(y + 1), z)].GetSunlight() ==
					         15))
						continue;
					m_sunlight_entries.push({{x, y, z}, sunlight});
				}
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
	    BlockMeshAlgo<BlockAlgoConfig<InnerPos1, BlockAlgoBound<InnerPos1>{0, 0, 0, kChunkSize, kChunkSize, kChunkSize},
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