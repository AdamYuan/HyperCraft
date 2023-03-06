#include <client/ChunkMesher.hpp>

#include <client/MeshEraser.hpp>
#include <client/WorldRenderer.hpp>

#include <bitset>
#include <queue>
#include <spdlog/spdlog.h>

#include <glm/gtx/string_cast.hpp>

#include <glm/gtc/type_ptr.hpp>

thread_local std::queue<ChunkMesher::LightEntry> ChunkMesher::m_light_queue;

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
static inline constexpr bool light_interfere(T x, T y, T z, LightLvl lvl) {
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

template <typename T, typename = std::enable_if_t<std::is_integral_v<T> && std::is_signed_v<T>>>
static inline constexpr bool chunk_xyz_extended15_in_bound(T x, T y, T z) {
	return !(x < -15 || x >= (int32_t)kChunkSize + 15 || y < -15 || y >= (int32_t)kChunkSize + 15 || z < -15 ||
	         z >= (int32_t)kChunkSize + 15);
}

void ChunkMesher::initial_sunlight_bfs() {
	while (!m_light_queue.empty()) {
		LightEntry e = m_light_queue.front();
		m_light_queue.pop();
		if (e.light_lvl <= 1u)
			continue;
		--e.light_lvl;
		for (BlockFace f = 0; f < 6; ++f) {
			LightEntry nei = e;
			BlockFaceProceed(glm::value_ptr(nei.position), f);

			if (!chunk_xyz_extended15_in_bound(nei.position.x, nei.position.y, nei.position.z))
				continue;

			uint32_t idx = chunk_xyz_extended15_to_index(nei.position.x, nei.position.y, nei.position.z);
			if (nei.light_lvl > m_light_buffer[idx].GetSunlight() &&
			    get_block(nei.position.x, nei.position.y, nei.position.z).GetIndirectLightPass()) {
				m_light_buffer[idx].SetSunlight(nei.light_lvl);
				if (light_interfere(nei.position.x, nei.position.y, nei.position.z, nei.light_lvl)) {
					m_light_queue.push(nei);
				}
			}
		}
	}
}

std::vector<ChunkMesher::MeshGenInfo> ChunkMesher::generate_mesh() const {
	std::vector<MeshGenInfo> ret;

	MeshGenInfo opaque_mesh_info, transparent_mesh_info;
	opaque_mesh_info.transparent = false;
	transparent_mesh_info.transparent = true;

	// deal with custom block mesh
	for (uint32_t idx = 0; idx < kChunkSize * kChunkSize * kChunkSize; ++idx) {
		Block b = m_chunk_ptr->GetBlock(idx);
		if (!b.HaveCustomMesh())
			continue;
		const BlockMesh *mesh = b.GetCustomMesh();
		glm::vec<3, int_fast8_t> pos{};
		ChunkIndex2XYZ(idx, glm::value_ptr(pos));
		glm::u32vec3 base = (glm::u32vec3)pos << ChunkMeshVertex::kUnitBitOffset;

		BlockFace cur_light_face = std::numeric_limits<BlockFace>::max();
		uint8_t cur_light_axis{}, u_light_axis{}, v_light_axis{};
		Light4 low_light4{}, high_light4{};
		for (uint32_t i = 0; i < mesh->face_count; ++i) {
			const BlockMeshFace *mesh_face = mesh->faces + i;

			if (mesh_face->light_face != cur_light_face) {
				cur_light_face = mesh_face->light_face;
				cur_light_axis = cur_light_face >> 1u;
				u_light_axis = (cur_light_axis + 1) % 3;
				v_light_axis = (cur_light_axis + 2) % 3;
				if (cur_light_face & 1u)
					std::swap(u_light_axis, v_light_axis);

				auto op_nei_pos = BlockFaceProceed(pos, BlockFaceOpposite(cur_light_face)),
				     nei_pos = BlockFaceProceed(pos, cur_light_face);
				light4_init(&low_light4, cur_light_face, op_nei_pos.x, op_nei_pos.y, op_nei_pos.z);
				if (get_block(nei_pos.x, nei_pos.y, nei_pos.z).GetIndirectLightPass())
					light4_init(&high_light4, cur_light_face, pos.x, pos.y, pos.z);
				else
					high_light4 = low_light4;
			}

			MeshGenInfo &info = mesh_face->texture.UseTransparentPass() ? transparent_mesh_info : opaque_mesh_info;
			// if indices would exceed, restart
			uint16_t cur_vertex = info.vertices.size();
			if (cur_vertex + 4 > UINT16_MAX) {
				bool trans = info.transparent;
				ret.push_back(std::move(info));
				info = MeshGenInfo{};
				info.transparent = trans;
			}

			for (const auto &vert : mesh_face->vertices) {
				info.aabb.Merge({base.x + vert.x, base.y + vert.y, base.z + vert.z});
				uint8_t du = vert.pos[u_light_axis], dv = vert.pos[v_light_axis],
				        dw = std::min(vert.pos[cur_light_axis], (uint8_t)ChunkMeshVertex::kUnitOffset);
				if (cur_light_face & 1u)
					dw = ChunkMeshVertex::kUnitOffset - (int32_t)dw;
				uint8_t ao = vert.ao, sunlight, torchlight;
				light4_interpolate(low_light4, high_light4, du, dv, dw, &ao, &sunlight, &torchlight);
				info.vertices.emplace_back(base.x + vert.x, base.y + vert.y, base.z + vert.z, mesh_face->axis,
				                           mesh_face->render_face, ao, sunlight, torchlight, mesh_face->texture.GetID(),
				                           mesh_face->texture.GetTransformation());
			}

			info.indices.push_back(cur_vertex);
			info.indices.push_back(cur_vertex + 1);
			info.indices.push_back(cur_vertex + 2);

			info.indices.push_back(cur_vertex);
			info.indices.push_back(cur_vertex + 2);
			info.indices.push_back(cur_vertex + 3);
		}
	}

	for (uint_fast8_t axis = 0; axis < 3; ++axis) {
		const uint_fast8_t u = (axis + 1) % 3;
		const uint_fast8_t v = (axis + 2) % 3;

		uint_fast8_t x[3]{0};
		int_fast8_t q[3]{0};
		thread_local static BlockTexture texture_mask[2][Chunk::kSize * Chunk::kSize]{};
		// thread_local static std::bitset<Chunk::kSize * Chunk::kSize> face_inv_mask{};
		thread_local static Light4 light_mask[2][Chunk::kSize * Chunk::kSize]{};

		// Compute texture_mask
		// TODO: enable dual-side textures in a slice
		uint8_t face_mask = 0;
		q[axis] = -1;
		std::fill(texture_mask[0], texture_mask[1] + kChunkSize * kChunkSize, 0);
		for (x[axis] = 0; x[axis] <= Chunk::kSize; ++x[axis]) {
			uint32_t counter = 0;
			for (x[v] = 0; x[v] < Chunk::kSize; ++x[v]) {
				for (x[u] = 0; x[u] < Chunk::kSize; ++x[u], ++counter) {
					const Block a = get_block(x[0] + q[0], x[1] + q[1], x[2] + q[2]);
					const Block b = get_block(x[0], x[1], x[2]);

					BlockFace f;
					if (x[axis] != 0 && a.ShowFace((f = axis << 1), b)) {
						texture_mask[0][counter] = a.GetTexture(f);
						face_mask |= 1u;
						light4_init(light_mask[0] + counter, f, (int_fast8_t)(x[0] + q[0]), (int_fast8_t)(x[1] + q[1]),
						            (int_fast8_t)(x[2] + q[2]));
					}
					if (Chunk::kSize != x[axis] && b.ShowFace((f = (axis << 1) | 1), a)) {
						texture_mask[1][counter] = b.GetTexture(f);
						face_mask |= 2u;
						light4_init(light_mask[1] + counter, f, (int_fast8_t)(x[0]), (int_fast8_t)(x[1]),
						            (int_fast8_t)(x[2]));
					}
				}
			}

			// Generate mesh for texture_mask using lexicographic ordering
			for (uint_fast8_t quad_face_inv = 0; quad_face_inv < 2; ++quad_face_inv) {
				if (!(face_mask & (1u << quad_face_inv)))
					continue;
				counter = 0;
				auto local_texture_mask = texture_mask[quad_face_inv];
				auto local_light_mask = light_mask[quad_face_inv];
				for (uint_fast8_t j = 0; j < Chunk::kSize; ++j) {
					for (uint_fast8_t i = 0; i < Chunk::kSize;) {
						const BlockTexture quad_texture = local_texture_mask[counter];
						if (!quad_texture.Empty()) {
							const Light4 quad_light = local_light_mask[counter];
							// Compute width
							uint_fast8_t width, height;
							for (width = 1; quad_texture == local_texture_mask[counter + width] &&
							                quad_light == local_light_mask[counter + width] && i + width < Chunk::kSize;
							     ++width)
								;

							// Compute height
							for (height = 1; j + height < Chunk::kSize; ++height)
								for (uint_fast8_t k = 0; k < width; ++k) {
									uint32_t idx = counter + k + height * Chunk::kSize;
									if (quad_texture != local_texture_mask[idx] ||
									    quad_light != local_light_mask[idx]) {
										goto end_height_loop;
									}
								}
						end_height_loop:

							// Add quad
							x[u] = i;
							x[v] = j;

							uint_fast8_t du[3] = {0}, dv[3] = {0};

							if (quad_face_inv) {
								du[v] = height;
								dv[u] = width;
							} else {
								dv[v] = height;
								du[u] = width;
							}

							// TODO: process resource rotation
							// if (quad_texture.GetRotation() == )

							MeshGenInfo &info =
							    quad_texture.UseTransparentPass() ? transparent_mesh_info : opaque_mesh_info;
							// if indices would exceed, restart
							uint16_t cur_vertex = info.vertices.size();
							if (cur_vertex + 4 > UINT16_MAX) {
								bool trans = info.transparent;
								ret.push_back(std::move(info));
								info = MeshGenInfo{};
								info.transparent = trans;
							}
							info.aabb.Merge({{uint32_t(x[0]) << ChunkMeshVertex::kUnitBitOffset,
							                  uint32_t(x[1]) << ChunkMeshVertex::kUnitBitOffset,
							                  uint32_t(x[2]) << ChunkMeshVertex::kUnitBitOffset},
							                 {uint32_t(x[0] + du[0] + dv[0]) << ChunkMeshVertex::kUnitBitOffset,
							                  uint32_t(x[1] + du[1] + dv[1]) << ChunkMeshVertex::kUnitBitOffset,
							                  uint32_t(x[2] + du[2] + dv[2]) << ChunkMeshVertex::kUnitBitOffset}});

							BlockFace quad_face = (axis << 1) | quad_face_inv;
							info.vertices.emplace_back(uint32_t(x[0]) << ChunkMeshVertex::kUnitBitOffset,
							                           uint32_t(x[1]) << ChunkMeshVertex::kUnitBitOffset,
							                           uint32_t(x[2]) << ChunkMeshVertex::kUnitBitOffset, axis,
							                           quad_face, quad_light.ao[0], quad_light.sunlight[0],
							                           quad_light.torchlight[0], quad_texture.GetID(),
							                           quad_texture.GetTransformation());
							info.vertices.emplace_back(uint32_t(x[0] + du[0]) << ChunkMeshVertex::kUnitBitOffset,
							                           uint32_t(x[1] + du[1]) << ChunkMeshVertex::kUnitBitOffset,
							                           uint32_t(x[2] + du[2]) << ChunkMeshVertex::kUnitBitOffset, axis,
							                           quad_face, quad_light.ao[1], quad_light.sunlight[1],
							                           quad_light.torchlight[1], quad_texture.GetID(),
							                           quad_texture.GetTransformation());
							info.vertices.emplace_back(
							    uint32_t(x[0] + du[0] + dv[0]) << ChunkMeshVertex::kUnitBitOffset,
							    uint32_t(x[1] + du[1] + dv[1]) << ChunkMeshVertex::kUnitBitOffset,
							    uint32_t(x[2] + du[2] + dv[2]) << ChunkMeshVertex::kUnitBitOffset, axis, quad_face,
							    quad_light.ao[2], quad_light.sunlight[2], quad_light.torchlight[2],
							    quad_texture.GetID(), quad_texture.GetTransformation());
							info.vertices.emplace_back(uint32_t(x[0] + dv[0]) << ChunkMeshVertex::kUnitBitOffset,
							                           uint32_t(x[1] + dv[1]) << ChunkMeshVertex::kUnitBitOffset,
							                           uint32_t(x[2] + dv[2]) << ChunkMeshVertex::kUnitBitOffset, axis,
							                           quad_face, quad_light.ao[3], quad_light.sunlight[3],
							                           quad_light.torchlight[3], quad_texture.GetID(),
							                           quad_texture.GetTransformation());

							if (quad_light.GetFlip()) {
								// 11--------10
								//|       / |
								//|    /    |
								//| /       |
								// 00--------01
								info.indices.push_back(cur_vertex);
								info.indices.push_back(cur_vertex + 1);
								info.indices.push_back(cur_vertex + 2);

								info.indices.push_back(cur_vertex);
								info.indices.push_back(cur_vertex + 2);
								info.indices.push_back(cur_vertex + 3);
							} else {
								// 11--------10
								//| \       |
								//|    \    |
								//|       \ |
								// 00--------01
								info.indices.push_back(cur_vertex + 1);
								info.indices.push_back(cur_vertex + 2);
								info.indices.push_back(cur_vertex + 3);

								info.indices.push_back(cur_vertex);
								info.indices.push_back(cur_vertex + 1);
								info.indices.push_back(cur_vertex + 3);
							}

							for (uint_fast8_t a = 0; a < height; ++a) {
								auto *base_ptr = local_texture_mask + counter + kChunkSize * a;
								std::fill(base_ptr, base_ptr + width, 0);
							}

							// Increase counters
							i += width;
							counter += width;
						} else {
							++i;
							++counter;
						}
					}
				}
			}
		}
	}
	if (!opaque_mesh_info.vertices.empty())
		ret.push_back(std::move(opaque_mesh_info));
	if (!transparent_mesh_info.vertices.empty())
		ret.push_back(std::move(transparent_mesh_info));
	return ret;
}

void ChunkMesher::light4_init(Light4 *light4, BlockFace face, int_fast8_t x, int_fast8_t y, int_fast8_t z) const {
	//  structure of the neighbour arrays
	// y
	// |
	// |  6   15  24
	// |    7   16  25
	// |      8   17  26
	// |
	// |  3   12  21
	// |    4   13  22
	// |      5   14  23
	// \-------------------x
	//  \ 0   9   18
	//   \  1   10  19
	//    \   2   11  20
	//     z

	/*constexpr uint32_t kLookup3[6][4][3] = {
	    {{21, 18, 19}, {21, 24, 25}, {23, 26, 25}, {23, 20, 19}}, {{3, 0, 1}, {5, 2, 1}, {5, 8, 7}, {3, 6, 7}},
	    {{15, 6, 7}, {17, 8, 7}, {17, 26, 25}, {15, 24, 25}},     {{9, 0, 1}, {9, 18, 19}, {11, 20, 19}, {11, 2,
	1}},
	    {{11, 2, 5}, {11, 20, 23}, {17, 26, 23}, {17, 8, 5}},     {{9, 0, 3}, {15, 6, 3}, {15, 24, 21}, {9, 18,
	21}}}; constexpr uint32_t kLookup1[6] = {22, 4, 16, 10, 14, 12};*/

	constexpr int_fast8_t kLookup1v[6][3] = {{1, 0, 0}, {-1, 0, 0}, {0, 1, 0}, {0, -1, 0}, {0, 0, 1}, {0, 0, -1}};
	constexpr int_fast8_t kLookup3v[6][4][3][3] = {{
	                                                   {{1, 0, -1}, {1, -1, -1}, {1, -1, 0}},
	                                                   {{1, 0, -1}, {1, 1, -1}, {1, 1, 0}},
	                                                   {{1, 0, 1}, {1, 1, 1}, {1, 1, 0}},
	                                                   {{1, 0, 1}, {1, -1, 1}, {1, -1, 0}},
	                                               },
	                                               {
	                                                   {{-1, 0, -1}, {-1, -1, -1}, {-1, -1, 0}},
	                                                   {{-1, 0, 1}, {-1, -1, 1}, {-1, -1, 0}},
	                                                   {{-1, 0, 1}, {-1, 1, 1}, {-1, 1, 0}},
	                                                   {{-1, 0, -1}, {-1, 1, -1}, {-1, 1, 0}},
	                                               },
	                                               {
	                                                   {{0, 1, -1}, {-1, 1, -1}, {-1, 1, 0}},
	                                                   {{0, 1, 1}, {-1, 1, 1}, {-1, 1, 0}},
	                                                   {{0, 1, 1}, {1, 1, 1}, {1, 1, 0}},
	                                                   {{0, 1, -1}, {1, 1, -1}, {1, 1, 0}},
	                                               },
	                                               {
	                                                   {{0, -1, -1}, {-1, -1, -1}, {-1, -1, 0}},
	                                                   {{0, -1, -1}, {1, -1, -1}, {1, -1, 0}},
	                                                   {{0, -1, 1}, {1, -1, 1}, {1, -1, 0}},
	                                                   {{0, -1, 1}, {-1, -1, 1}, {-1, -1, 0}},
	                                               },
	                                               {
	                                                   {{0, -1, 1}, {-1, -1, 1}, {-1, 0, 1}},
	                                                   {{0, -1, 1}, {1, -1, 1}, {1, 0, 1}},
	                                                   {{0, 1, 1}, {1, 1, 1}, {1, 0, 1}},
	                                                   {{0, 1, 1}, {-1, 1, 1}, {-1, 0, 1}},
	                                               },
	                                               {{{0, -1, -1}, {-1, -1, -1}, {-1, 0, -1}},
	                                                {{0, 1, -1}, {-1, 1, -1}, {-1, 0, -1}},
	                                                {{0, 1, -1}, {1, 1, -1}, {1, 0, -1}},
	                                                {{0, -1, -1}, {1, -1, -1}, {1, 0, -1}}}};

	bool indirect_pass[3], direct_pass[4];

	// TODO: optimize this

	for (uint32_t v = 0; v < 4; ++v) {
		for (uint32_t b = 0; b < 3; ++b) {
			Block blk =
			    get_block(x + kLookup3v[face][v][b][0], y + kLookup3v[face][v][b][1], z + kLookup3v[face][v][b][2]);
			indirect_pass[b] = blk.GetIndirectLightPass();
			direct_pass[b] = blk.GetVerticalLightPass() || blk.GetCollisionMask() != BlockCollisionBits::kSolid;
		}
		{
			Block blk = get_block(x + kLookup1v[face][0], y + kLookup1v[face][1], z + kLookup1v[face][2]);
			direct_pass[3] = blk.GetVerticalLightPass() || blk.GetCollisionMask() != BlockCollisionBits::kSolid;
		}

		uint32_t ao =
		    (!direct_pass[0] && !direct_pass[2]) || (!direct_pass[1] && !direct_pass[3])
		        ? 0u
		        : (uint32_t)std::max(3 - !direct_pass[0] - !direct_pass[1] - !direct_pass[2] - !direct_pass[3], 0);
		light4->ao.Set(v, ao);

		// smooth the LightLvl using the average value
		uint32_t counter = 1;
		Light light = m_light_buffer[chunk_xyz_extended15_to_index(x + kLookup1v[face][0], y + kLookup1v[face][1],
		                                                           z + kLookup1v[face][2])];
		uint32_t sunlight_sum = light.GetSunlight(), torchlight_sum = light.GetTorchlight();
		if (indirect_pass[0] || indirect_pass[2]) {
			if (indirect_pass[0]) {
				++counter;
				light = m_light_buffer[chunk_xyz_extended15_to_index(
				    x + kLookup3v[face][v][0][0], y + kLookup3v[face][v][0][1], z + kLookup3v[face][v][0][2])];
				sunlight_sum += light.GetSunlight();
				torchlight_sum += light.GetTorchlight();
			}
			if (indirect_pass[1]) {
				++counter;
				light = m_light_buffer[chunk_xyz_extended15_to_index(
				    x + kLookup3v[face][v][1][0], y + kLookup3v[face][v][1][1], z + kLookup3v[face][v][1][2])];
				sunlight_sum += light.GetSunlight();
				torchlight_sum += light.GetTorchlight();
			}
			if (indirect_pass[2]) {
				++counter;
				light = m_light_buffer[chunk_xyz_extended15_to_index(
				    x + kLookup3v[face][v][2][0], y + kLookup3v[face][v][2][1], z + kLookup3v[face][v][2][2])];
				sunlight_sum += light.GetSunlight();
				torchlight_sum += light.GetTorchlight();
			}
		}
		light4->sunlight[v] = (sunlight_sum << 2u) / counter;
		light4->torchlight[v] = (torchlight_sum << 2u) / counter;
	}
}

void ChunkMesher::light4_interpolate(const ChunkMesher::Light4 &low_light, const ChunkMesher::Light4 &high_light,
                                     uint8_t du, uint8_t dv, uint8_t dw, uint8_t *ao, uint8_t *sunlight,
                                     uint8_t *torchlight) {
// don't care about the macro cost since this will be optimized
#define LERP(a, b, t) \
	((uint32_t)(a)*uint32_t((int32_t)ChunkMeshVertex::kUnitOffset - (int32_t)(t)) + (uint32_t)(b) * (uint32_t)(t))
#define CEIL(a, b) ((a) / (b) + ((a) % (b) ? 1 : 0))

	if (*ao == 4) {
		uint32_t ao_sum = LERP(
		    LERP(LERP(low_light.ao[0], low_light.ao[1], du), LERP(low_light.ao[3], low_light.ao[2], du), dv),
		    LERP(LERP(high_light.ao[0], high_light.ao[1], du), LERP(high_light.ao[3], high_light.ao[2], du), dv), dw);
		*ao = CEIL(ao_sum, ChunkMeshVertex::kUnitOffset * ChunkMeshVertex::kUnitOffset * ChunkMeshVertex::kUnitOffset);
	}

	uint32_t sunlight_sum = LERP(LERP(LERP(low_light.sunlight[0], low_light.sunlight[1], du),
	                                  LERP(low_light.sunlight[3], low_light.sunlight[2], du), dv),
	                             LERP(LERP(high_light.sunlight[0], high_light.sunlight[1], du),
	                                  LERP(high_light.sunlight[3], high_light.sunlight[2], du), dv),
	                             dw);
	*sunlight =
	    CEIL(sunlight_sum, ChunkMeshVertex::kUnitOffset * ChunkMeshVertex::kUnitOffset * ChunkMeshVertex::kUnitOffset);
	uint32_t torchlight_sum = LERP(LERP(LERP(low_light.torchlight[0], low_light.torchlight[1], du),
	                                    LERP(low_light.torchlight[3], low_light.torchlight[2], du), dv),
	                               LERP(LERP(high_light.torchlight[0], high_light.torchlight[1], du),
	                                    LERP(high_light.torchlight[3], high_light.torchlight[2], du), dv),
	                               dw);
	*torchlight = CEIL(torchlight_sum,
	                   ChunkMeshVertex::kUnitOffset * ChunkMeshVertex::kUnitOffset * ChunkMeshVertex::kUnitOffset);

#undef CEIL
#undef LERP
}

void ChunkMesher::Run() {
	if (!lock())
		return;

	if (!m_chunk_ptr->IsGenerated() /* || !m_chunk_ptr->IsLatestLight()*/)
		return;

	uint64_t version = m_chunk_ptr->FetchMeshVersion();
	if (!version)
		return;
	// if the neighbour chunks are not totally generated, return and move it back
	for (const auto &i : m_neighbour_chunk_ptr)
		if (!i->IsGenerated()) {
			// spdlog::info("remesh");
			push_worker(ChunkMesher::CreateWithInitialLight(m_chunk_ptr));
			return;
		}

	if (m_init_light) {
		m_chunk_ptr->PendLightVersion();
		uint64_t light_version = m_chunk_ptr->FetchLightVersion();
		if (light_version) {
			// TODO: optimize this
			for (int8_t y = -15; y < (int8_t)kChunkSize + 15; ++y)
				for (int8_t z = -15; z < (int8_t)kChunkSize + 15; ++z)
					for (int8_t x = -15; x < (int8_t)kChunkSize + 15; ++x) {
						Light light = get_light(x, y, z);
						m_light_buffer[chunk_xyz_extended15_to_index(x, y, z)] = light;
						if (light_interfere(x, y, z, light.GetSunlight()))
							m_light_queue.push({{x, y, z}, light.GetSunlight()});
						// if (light_interfere(x, y, z, light.GetTorchlight()))
						//	torchlight_queue.Push({{x, y, z}, light.GetTorchlight()});
					}
			initial_sunlight_bfs();
			m_chunk_ptr->PushLight(light_version, m_light_buffer);
		}
	}

	std::vector<MeshGenInfo> meshes = generate_mesh();
	std::shared_ptr<World> world_ptr = m_chunk_ptr->LockWorld();
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
		    world_renderer_ptr->GetChunkRenderer(), info.vertices, info.indices,
		    {(fAABB)info.aabb / glm::vec3(1u << ChunkMeshVertex::kUnitBitOffset) + (glm::vec3)base_position,
		     base_position, (uint32_t)info.transparent});
	}
	// Push mesh to chunk
	m_chunk_ptr->SwapMesh(version, mesh_handles);
	// if (!mesh_handles.empty())
	// 	MeshEraser{std::move(mesh_handles)}.Run();
}
