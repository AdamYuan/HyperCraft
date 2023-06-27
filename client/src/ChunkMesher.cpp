#include <client/ChunkMesher.hpp>

#include <client/WorldRenderer.hpp>

#include <queue>
#include <spdlog/spdlog.h>

#include <client/BlockLightAlgo.hpp>
#include <client/BlockMeshAlgo.hpp>

namespace hc::client {

template <typename T, typename = std::enable_if_t<std::is_integral_v<T> && std::is_signed_v<T>>>
static inline constexpr uint32_t chunk_xyz_extended15_to_index(T x, T y, T z) {
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

template <typename T, typename = std::enable_if_t<std::is_integral_v<T> && std::is_signed_v<T>>>
static inline constexpr bool light_interfere(T x, T y, T z, block::LightLvl lvl) {
	if (lvl <= 1)
		return false;
	uint32_t dist = 0;
	if (x < 0 || x >= kChunkSize)
		dist += x < 0 ? -x : x - (int32_t)kChunkSize;
	if (y < 0 || y >= kChunkSize)
		dist += y < 0 ? -y : y - (int32_t)kChunkSize;
	if (z < 0 || z >= kChunkSize)
		dist += z < 0 ? -z : z - (int32_t)kChunkSize;
	return (uint32_t)lvl >= dist;
}

void ChunkMesher::Run() {
	if (!lock())
		return;

	// if the neighbour chunks are not totally generated, return and move it back
	for (const auto &i : m_neighbour_chunk_ptr)
		if (!i->IsGenerated()) {
			try_push_worker(ChunkMesher::TryCreateWithInitialLight(m_chunk_ptr));
			return;
		}

	if (m_init_light) {
		using LightAlgo = BlockLightAlgo<
		    BlockAlgoConfig<int32_t, BlockAlgoBound<int32_t>{-1, -1, -1, (int32_t)kChunkSize + 1,
		                                                     (int32_t)kChunkSize + 1, (int32_t)kChunkSize + 1}>,
		    14>;

		LightAlgo::Queue sunlight_entries, torchlight_entries;
		for (int32_t y = -15; y < (int32_t)kChunkSize + 15; ++y)
			for (int32_t z = -15; z < (int32_t)kChunkSize + 15; ++z)
				for (int32_t x = -15; x < (int32_t)kChunkSize + 15; ++x) {
					block::Light light = get_light(x, y, z);
					m_light_buffer[chunk_xyz_extended15_to_index(x, y, z)] = light;
					if (LightAlgo::IsBorderLightInterfere(x, y, z, light.GetSunlight()))
						sunlight_entries.push({{x, y, z}, light.GetSunlight()});
					if (LightAlgo::IsBorderLightInterfere(x, y, z, light.GetTorchlight()))
						torchlight_entries.push({{x, y, z}, light.GetTorchlight()});
				}

		LightAlgo algo{};
		algo.PropagateLight(
		    std::move(sunlight_entries), [this](auto x, auto y, auto z) -> block::Block { return get_block(x, y, z); },
		    [](auto x, auto y, auto z) -> block::LightLvl {
			    return m_light_buffer[chunk_xyz_extended15_to_index(x, y, z)].GetSunlight();
		    },
		    [](auto x, auto y, auto z, block::LightLvl lvl) {
			    m_light_buffer[chunk_xyz_extended15_to_index(x, y, z)].SetSunlight(lvl);
		    });
		algo.PropagateLight(
		    std::move(torchlight_entries),
		    [this](auto x, auto y, auto z) -> block::Block { return get_block(x, y, z); },
		    [](auto x, auto y, auto z) -> block::LightLvl {
			    return m_light_buffer[chunk_xyz_extended15_to_index(x, y, z)].GetTorchlight();
		    },
		    [](auto x, auto y, auto z, block::LightLvl lvl) {
			    m_light_buffer[chunk_xyz_extended15_to_index(x, y, z)].SetTorchlight(lvl);
		    });
		m_chunk_ptr->PushLight(m_light_version, m_light_buffer);
	}

	std::vector<BlockMesh> meshes =
	    BlockMeshAlgo<BlockAlgoConfig<uint32_t, BlockAlgoBound<uint32_t>{0, 0, 0, kChunkSize, kChunkSize, kChunkSize},
	                                  kBlockAlgoSwizzleYZX>>{}
	        .Generate([this](auto x, auto y, auto z) -> block::Block { return get_block(x, y, z); },
	                  [](auto x, auto y, auto z) -> block::Light {
		                  return m_light_buffer[chunk_xyz_extended15_to_index(x, y, z)];
	                  });

	auto world_ptr = m_chunk_ptr->LockWorld();
	if (!world_ptr)
		return;
	std::shared_ptr<WorldRenderer> world_renderer_ptr = world_ptr->LockWorldRenderer();
	if (!world_renderer_ptr)
		return;

	m_chunk_ptr->SetMeshedFlag();

	glm::i32vec3 base_position = (glm::i32vec3)m_chunk_ptr->GetPosition() * (int32_t)Chunk::kSize;
	// erase previous meshes
	std::vector<std::unique_ptr<ChunkMeshHandle>> mesh_handles(meshes.size());
	// spdlog::info("Chunk {} (version {}) meshed with {} meshes", glm::to_string(m_chunk_ptr->GetPosition()),
	// version,meshes.size());
	for (uint32_t i = 0; i < meshes.size(); ++i) {
		auto &info = meshes[i];
		mesh_handles[i] = ChunkMeshHandle::Create(
		    world_renderer_ptr->GetChunkMeshPool(), info.vertices, info.indices,
		    {(fAABB)info.aabb / glm::vec3(1u << BlockVertex::kUnitBitOffset) + (glm::vec3)base_position, base_position,
		     (uint32_t)info.transparent});
	}

	// Push mesh to chunk
	m_chunk_ptr->PushMesh(m_version, mesh_handles);
}

} // namespace hc::client