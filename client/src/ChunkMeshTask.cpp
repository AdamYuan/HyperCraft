#include <client/ChunkTaskPool.hpp>

#include <client/BlockLightAlgo.hpp>
#include <client/BlockMeshAlgo.hpp>
#include <client/World.hpp>
#include <client/WorldRenderer.hpp>

#include <glm/gtx/string_cast.hpp>

namespace hc::client {

template <typename T, typename = std::enable_if_t<std::is_integral_v<T> && std::is_signed_v<T>>>
static inline constexpr uint32_t chunk_xyz_extended15_to_index(T x, T y, T z) {
	bool x_inside = 0 <= x && x < kChunkSize, y_inside = 0 <= y && y < kChunkSize, z_inside = 0 <= z && z < kChunkSize;
	uint32_t bits = x_inside | (y_inside << 1u) | (z_inside << 2u);
	constexpr uint32_t kOffsets[7] = {
	    0,
	    30 * 30 * 30,
	    30 * 30 * 30 + 30 * kChunkSize * 30,
	    30 * 30 * 30 + 30 * kChunkSize * 30 * 2,
	    30 * 30 * 30 + 30 * kChunkSize * 30 * 2 + kChunkSize * kChunkSize * 30,
	    30 * 30 * 30 + 30 * kChunkSize * 30 * 3 + kChunkSize * kChunkSize * 30,
	    30 * 30 * 30 + 30 * kChunkSize * 30 * 3 + kChunkSize * kChunkSize * 30 * 2,
	};
	constexpr uint32_t kMultipliers[7][3] = {
	    {30, 30, 30},         {kChunkSize, 30, 30},         {30, kChunkSize, 30},         {kChunkSize, kChunkSize, 30},
	    {30, 30, kChunkSize}, {kChunkSize, 30, kChunkSize}, {30, kChunkSize, kChunkSize},
	};
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

	// printf("Try Mesh\n");

	std::array<std::shared_ptr<Chunk>, 27> chunks;

	for (uint32_t i = 0; i < 27; ++i) {
		ChunkPos3 nei_pos;
		Chunk::NeighbourIndex2CmpXYZ(i, glm::value_ptr(nei_pos));
		nei_pos += chunk_pos;

		if (task_pool.AnyQueued<ChunkTaskType::kGenerate, ChunkTaskType::kLight>(nei_pos))
			return std::nullopt;

		std::shared_ptr<Chunk> nei_chunk = task_pool.GetWorld().FindChunk(nei_pos);
		if (nei_chunk == nullptr)
			return std::nullopt;
		chunks[i] = std::move(nei_chunk);
	}

	bool init_light = m_init_light;

	m_queued = m_init_light = false;
	return ChunkTaskRunnerData<ChunkTaskType::kMesh>{std::move(chunks), init_light};
}

void ChunkTaskRunner<ChunkTaskType::kMesh>::Run(ChunkTaskPool *p_task_pool,
                                                ChunkTaskRunnerData<ChunkTaskType::kMesh> &&data) {
	const auto &neighbour_chunks = data.GetChunkPtrArray();
	const auto &chunk = neighbour_chunks.back();

	std::vector<BlockMesh> meshes;

	if (data.ShouldInitLight()) {
		using LightAlgo = BlockLightAlgo<
		    BlockAlgoConfig<int32_t, BlockAlgoBound<int32_t>{-1, -1, -1, (int32_t)kChunkSize + 1,
		                                                     (int32_t)kChunkSize + 1, (int32_t)kChunkSize + 1}>,
		    14>;

		LightAlgo::Queue sunlight_entries, torchlight_entries;
		for (int32_t y = -15; y < (int32_t)kChunkSize + 15; ++y)
			for (int32_t z = -15; z < (int32_t)kChunkSize + 15; ++z)
				for (int32_t x = -15; x < (int32_t)kChunkSize + 15; ++x) {
					block::Light light;
					if (Chunk::IsValidPosition(x, y, z)) {
						light = chunk->GetLight(x, y, z);
					} else {
						uint32_t nei_idx = Chunk::GetBlockNeighbourIndex(x, y, z);
						light = neighbour_chunks[nei_idx]->GetLightFromNeighbour(x, y, z);
						m_extend_light_buffer[chunk_xyz_extended15_to_index(x, y, z)] = light;
					}
					if (LightAlgo::IsBorderLightInterfere(x, y, z, light.GetSunlight()))
						sunlight_entries.push({{x, y, z}, light.GetSunlight()});
					if (LightAlgo::IsBorderLightInterfere(x, y, z, light.GetTorchlight()))
						torchlight_entries.push({{x, y, z}, light.GetTorchlight()});
				}

		LightAlgo algo{};
		algo.PropagateLight(
		    std::move(sunlight_entries),
		    [&neighbour_chunks](auto x, auto y, auto z) -> block::Block {
			    return neighbour_chunks[Chunk::GetBlockNeighbourIndex(x, y, z)]->GetBlockFromNeighbour(x, y, z);
		    },
		    [this, &chunk](auto x, auto y, auto z) -> block::LightLvl {
			    return Chunk::IsValidPosition(x, y, z)
			               ? chunk->GetLightRef(x, y, z).GetSunlight()
			               : m_extend_light_buffer[chunk_xyz_extended15_to_index(x, y, z)].GetSunlight();
		    },
		    [this, &chunk](auto x, auto y, auto z, block::LightLvl lvl) {
			    Chunk::IsValidPosition(x, y, z)
			        ? chunk->GetLightRef(x, y, z).SetSunlight(lvl)
			        : m_extend_light_buffer[chunk_xyz_extended15_to_index(x, y, z)].SetSunlight(lvl);
		    });
		algo.PropagateLight(
		    std::move(torchlight_entries),
		    [&neighbour_chunks](auto x, auto y, auto z) -> block::Block {
			    return neighbour_chunks[Chunk::GetBlockNeighbourIndex(x, y, z)]->GetBlockFromNeighbour(x, y, z);
		    },
		    [this, &chunk](auto x, auto y, auto z) -> block::LightLvl {
			    return Chunk::IsValidPosition(x, y, z)
			               ? chunk->GetLight(x, y, z).GetTorchlight()
			               : m_extend_light_buffer[chunk_xyz_extended15_to_index(x, y, z)].GetTorchlight();
		    },
		    [this, &chunk](auto x, auto y, auto z, block::LightLvl lvl) {
			    Chunk::IsValidPosition(x, y, z)
			        ? chunk->GetLight(x, y, z).SetTorchlight(lvl)
			        : m_extend_light_buffer[chunk_xyz_extended15_to_index(x, y, z)].SetTorchlight(lvl);
		    });

		meshes = BlockMeshAlgo<BlockAlgoConfig<
		    uint32_t, BlockAlgoBound<uint32_t>{0, 0, 0, kChunkSize, kChunkSize, kChunkSize}, kBlockAlgoSwizzleYZX>>{}
		             .Generate(
		                 [&neighbour_chunks](auto x, auto y, auto z) -> block::Block {
			                 return neighbour_chunks[Chunk::GetBlockNeighbourIndex(x, y, z)]->GetBlockFromNeighbour(
			                     x, y, z);
		                 },
		                 [this, &chunk](auto x, auto y, auto z) -> block::Light {
			                 return Chunk::IsValidPosition(x, y, z)
			                            ? chunk->GetLight(x, y, z)
			                            : m_extend_light_buffer[chunk_xyz_extended15_to_index(x, y, z)];
		                 });
	} else {
		meshes = BlockMeshAlgo<BlockAlgoConfig<
		    uint32_t, BlockAlgoBound<uint32_t>{0, 0, 0, kChunkSize, kChunkSize, kChunkSize}, kBlockAlgoSwizzleYZX>>{}
		             .Generate(
		                 [&neighbour_chunks](auto x, auto y, auto z) -> block::Block {
			                 return neighbour_chunks[Chunk::GetBlockNeighbourIndex(x, y, z)]->GetBlockFromNeighbour(
			                     x, y, z);
		                 },
		                 [&neighbour_chunks](auto x, auto y, auto z) -> block::Light {
			                 return neighbour_chunks[Chunk::GetBlockNeighbourIndex(x, y, z)]->GetLightFromNeighbour(
			                     x, y, z);
		                 });
	}

	auto renderer = p_task_pool->GetWorld().LockRenderer();
	if (!renderer)
		return;

	glm::i32vec3 base_position = (glm::i32vec3)chunk->GetPosition() * (int32_t)Chunk::kSize;
	// erase previous meshes
	std::vector<std::unique_ptr<ChunkMeshHandle>> mesh_handles(meshes.size());
	// spdlog::info("Chunk {} (version {}) meshed with {} meshes", glm::to_string(m_chunk_ptr->GetPosition()),
	// version,meshes.size());
	for (uint32_t i = 0; i < meshes.size(); ++i) {
		auto &info = meshes[i];
		mesh_handles[i] = ChunkMeshHandle::Create(
		    renderer->GetChunkMeshPool(), info.vertices, info.indices,
		    {(fAABB)info.aabb / glm::vec3(1u << BlockVertex::kUnitBitOffset) + (glm::vec3)base_position, base_position,
		     (uint32_t)info.transparent});
	}
	chunk->SetMesh(std::move(mesh_handles));
}

} // namespace hc::client