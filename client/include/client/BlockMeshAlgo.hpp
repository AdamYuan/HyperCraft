#pragma once

#include "BlockAlgo.hpp"
#include "BlockVertex.hpp"

#include <block/Block.hpp>
#include <block/Light.hpp>
#include <common/Size.hpp>

#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/hash.hpp>

#include <vector>

namespace hc::client {

template <typename Config> class BlockMeshAlgo {
private:
	struct AO4 { // compressed ambient occlusion data for 4 vertices (a face)
		uint8_t m_data;
		inline uint8_t Get(uint32_t i) const { return (m_data >> (i << 1u)) & 0x3u; }
		inline void Set(uint32_t i, uint8_t ao) {
			uint8_t mask = ~(0x3u << (i << 1u));
			m_data = (m_data & mask) | (ao << (i << 1u));
		}
		inline uint8_t operator[](uint32_t i) const { return Get(i); }
		inline bool operator==(AO4 r) const { return m_data == r.m_data; }
		inline bool operator!=(AO4 r) const { return m_data != r.m_data; }
	};
	struct Light4 { // compressed light data for 4 vertices (a face)
		union {
			struct {
				uint8_t sunlight[4], torchlight[4];
			};
			uint64_t lights;
		};
		AO4 ao;
		inline bool GetFlip() const {
			return std::abs((int32_t)(ao[0] + 1) * (sunlight[0] + 1) - (ao[2] + 1) * (sunlight[2] + 1))
			       // + std::max(m_light[0].GetTorchlight(), m_light[2].GetTorchlight())
			       < std::abs((int32_t)(ao[1] + 1) * (sunlight[1] + 1) - (ao[3] + 1) * (sunlight[3] + 1));
			// + std::max(m_light[1].GetTorchlight(), m_light[3].GetTorchlight());
		}
		inline bool operator==(Light4 f) const { return ao == f.ao && lights == f.lights; }
		inline bool operator!=(Light4 f) const { return ao != f.ao || lights != f.lights; }
	};

	template <typename GetBlockFunc, typename GetLightFunc>
	inline static void light4_init(GetBlockFunc &&get_block_func, GetLightFunc &get_light_func, Light4 *light4,
	                               block::BlockFace face, int32_t x, int32_t y, int32_t z) {
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

		constexpr int8_t kLookup1v[6][3] = {{1, 0, 0}, {-1, 0, 0}, {0, 1, 0}, {0, -1, 0}, {0, 0, 1}, {0, 0, -1}};
		constexpr int8_t kLookup3v[6][4][3][3] = {{
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
				block::Block blk = get_block_func(x + kLookup3v[face][v][b][0], y + kLookup3v[face][v][b][1],
				                                  z + kLookup3v[face][v][b][2]);
				indirect_pass[b] = blk.GetIndirectLightPass();
				direct_pass[b] = blk.GetVerticalLightPass() || blk.GetCollision() != block::BlockCollision::kSolid;
			}
			{
				block::Block blk =
				    get_block_func(x + kLookup1v[face][0], y + kLookup1v[face][1], z + kLookup1v[face][2]);
				direct_pass[3] = blk.GetVerticalLightPass() || blk.GetCollision() != block::BlockCollision::kSolid;
			}

			uint32_t ao =
			    (!direct_pass[0] && !direct_pass[2]) || (!direct_pass[1] && !direct_pass[3])
			        ? 0u
			        : (uint32_t)std::max(3 - !direct_pass[0] - !direct_pass[1] - !direct_pass[2] - !direct_pass[3], 0);
			light4->ao.Set(v, ao);

			// smooth the LightLvl using the average value
			uint32_t counter = 1;
			block::Light light = get_light_func(x + kLookup1v[face][0], y + kLookup1v[face][1], z + kLookup1v[face][2]);
			uint32_t sunlight_sum = light.GetSunlight(), torchlight_sum = light.GetTorchlight();
			if (indirect_pass[0] || indirect_pass[2]) {
				if (indirect_pass[0]) {
					++counter;
					light = get_light_func(x + kLookup3v[face][v][0][0], y + kLookup3v[face][v][0][1],
					                       z + kLookup3v[face][v][0][2]);
					sunlight_sum += light.GetSunlight();
					torchlight_sum += light.GetTorchlight();
				}
				if (indirect_pass[1]) {
					++counter;
					light = get_light_func(x + kLookup3v[face][v][1][0], y + kLookup3v[face][v][1][1],
					                       z + kLookup3v[face][v][1][2]);
					sunlight_sum += light.GetSunlight();
					torchlight_sum += light.GetTorchlight();
				}
				if (indirect_pass[2]) {
					++counter;
					light = get_light_func(x + kLookup3v[face][v][2][0], y + kLookup3v[face][v][2][1],
					                       z + kLookup3v[face][v][2][2]);
					sunlight_sum += light.GetSunlight();
					torchlight_sum += light.GetTorchlight();
				}
			}
			light4->sunlight[v] = (sunlight_sum << 2u) / counter;
			light4->torchlight[v] = (torchlight_sum << 2u) / counter;
		}
	}

	inline static void light4_interpolate(const Light4 &low_light, const Light4 &high_light, uint8_t du, uint8_t dv,
	                                      uint8_t dw, uint8_t *ao, uint8_t *sunlight, uint8_t *torchlight) {
// don't care about the macro cost since this will be optimized
#define LERP(a, b, t) \
	((uint32_t)(a)*uint32_t((int32_t)BlockVertex::kUnitOffset - (int32_t)(t)) + (uint32_t)(b) * (uint32_t)(t))
#define CEIL(a, b) ((a) / (b) + ((a) % (b) ? 1 : 0))

		if (*ao == 4) {
			uint32_t ao_sum = LERP(
			    LERP(LERP(low_light.ao[0], low_light.ao[1], du), LERP(low_light.ao[3], low_light.ao[2], du), dv),
			    LERP(LERP(high_light.ao[0], high_light.ao[1], du), LERP(high_light.ao[3], high_light.ao[2], du), dv),
			    dw);
			*ao = CEIL(ao_sum, BlockVertex::kUnitOffset * BlockVertex::kUnitOffset * BlockVertex::kUnitOffset);
		}

		uint32_t sunlight_sum = LERP(LERP(LERP(low_light.sunlight[0], low_light.sunlight[1], du),
		                                  LERP(low_light.sunlight[3], low_light.sunlight[2], du), dv),
		                             LERP(LERP(high_light.sunlight[0], high_light.sunlight[1], du),
		                                  LERP(high_light.sunlight[3], high_light.sunlight[2], du), dv),
		                             dw);
		*sunlight = CEIL(sunlight_sum, BlockVertex::kUnitOffset * BlockVertex::kUnitOffset * BlockVertex::kUnitOffset);
		uint32_t torchlight_sum = LERP(LERP(LERP(low_light.torchlight[0], low_light.torchlight[1], du),
		                                    LERP(low_light.torchlight[3], low_light.torchlight[2], du), dv),
		                               LERP(LERP(high_light.torchlight[0], high_light.torchlight[1], du),
		                                    LERP(high_light.torchlight[3], high_light.torchlight[2], du), dv),
		                               dw);
		*torchlight =
		    CEIL(torchlight_sum, BlockVertex::kUnitOffset * BlockVertex::kUnitOffset * BlockVertex::kUnitOffset);

#undef CEIL
#undef LERP
	}

	template <typename GetBlockFunc, typename GetLightFunc>
	inline void generate_custom_mesh(GetBlockFunc &&get_block_func, GetLightFunc &&get_light_func) {
		using T = typename Config::Type;

		for (T a1 = Config::template GetMin<Config::kA1>() - 1; a1 <= Config::template GetMax<Config::kA1>(); ++a1) {
			bool in1 = Config::template GetMin<Config::kA1>() <= a1 && a1 < Config::template GetMax<Config::kA1>();
			for (T a2 = Config::template GetMin<Config::kA2>() - 1; a2 <= Config::template GetMax<Config::kA2>();
			     ++a2) {
				bool in2 = Config::template GetMin<Config::kA2>() <= a2 && a2 < Config::template GetMax<Config::kA2>();
				for (T a3 = Config::template GetMin<Config::kA3>() - 1; a3 <= Config::template GetMax<Config::kA3>();
				     ++a3) {
					bool in3 =
					    Config::template GetMin<Config::kA3>() <= a3 && a3 < Config::template GetMax<Config::kA3>();

					auto [x, y, z] = Config::ToXYZ(a1, a2, a3);
					block::Block b = get_block_func(x, y, z);

					const block::BlockMesh *mesh = b.GetCustomMesh();
					if (mesh == nullptr)
						continue;

					uint32_t face_count = mesh->face_count;
					const block::BlockMeshFace *faces = mesh->faces;
					if (mesh->p_dynamic_mesh_func) {
						for (uint32_t i = 0; i < mesh->fetch_neighbour_count; ++i) {
							const auto &dp = mesh->fetch_neighbours[i];
							m_neighbour_block_buffer[i] = get_block_func(x + dp.x, y + dp.y, z + dp.z);
						}
						bool use_dynamic_texture = false;
						mesh->p_dynamic_mesh_func(m_neighbour_block_buffer, m_dynamic_block_mesh_buffer, &face_count,
						                          m_dynamic_block_texture_buffer.data(), &use_dynamic_texture);

						if (use_dynamic_texture) {
							m_dynamic_block_textures[glm::vec<3, T>{x, y, z}] =
							    std::array<texture::BlockTexture, 6>{m_dynamic_block_texture_buffer};
						}

						faces = m_dynamic_block_mesh_buffer;
					}

					// Not a inner block, just get its face textures
					if (!in1 || !in2 || !in3)
						continue;

					glm::vec<3, int32_t> pos{x, y, z};
					glm::u32vec3 base = glm::u32vec3{x, y, z} << BlockVertex::kUnitBitOffset;

					auto cur_light_face = std::numeric_limits<block::BlockFace>::max();
					uint8_t cur_light_axis{}, u_light_axis{}, v_light_axis{};
					Light4 low_light4{}, high_light4{};
					for (uint32_t i = 0; i < face_count; ++i) {
						const block::BlockMeshFace *mesh_face = faces + i;

						if (mesh_face->light_face != cur_light_face) {
							cur_light_face = mesh_face->light_face;
							cur_light_axis = cur_light_face >> 1u;
							u_light_axis = (cur_light_axis + 1) % 3;
							v_light_axis = (cur_light_axis + 2) % 3;
							if (cur_light_face & 1u)
								std::swap(u_light_axis, v_light_axis);

							auto op_nei_pos = block::BlockFaceProceed(pos, block::BlockFaceOpposite(cur_light_face)),
							     nei_pos = block::BlockFaceProceed(pos, cur_light_face);
							light4_init(get_block_func, get_light_func, &low_light4, cur_light_face, op_nei_pos.x,
							            op_nei_pos.y, op_nei_pos.z);
							if (get_block_func(nei_pos.x, nei_pos.y, nei_pos.z).GetIndirectLightPass())
								light4_init(get_block_func, get_light_func, &high_light4, cur_light_face, pos.x, pos.y,
								            pos.z);
							else
								high_light4 = low_light4;
						}

						BlockMesh &info =
						    mesh_face->texture.UseTransparentPass() ? m_transparent_mesh_info : m_opaque_mesh_info;
						// if indices would exceed, restart
						uint16_t cur_vertex = info.vertices.size();
						if (cur_vertex + 4 > UINT16_MAX) {
							bool trans = info.transparent;
							m_meshes.push_back(std::move(info));
							info = BlockMesh{};
							info.transparent = trans;
						}

						for (const auto &vert : mesh_face->vertices) {
							info.aabb.Merge({base.x + vert.x, base.y + vert.y, base.z + vert.z});
							uint8_t du = vert.pos[u_light_axis], dv = vert.pos[v_light_axis],
							        dw = std::min(vert.pos[cur_light_axis], (uint8_t)BlockVertex::kUnitOffset);
							if (cur_light_face & 1u)
								dw = BlockVertex::kUnitOffset - (int32_t)dw;
							uint8_t ao = vert.ao, sunlight, torchlight;
							light4_interpolate(low_light4, high_light4, du, dv, dw, &ao, &sunlight, &torchlight);
							info.vertices.emplace_back(base.x + vert.x, base.y + vert.y, base.z + vert.z,
							                           mesh_face->axis, mesh_face->render_face, ao, sunlight,
							                           torchlight, mesh_face->texture.GetID(),
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
			}
		}
	}

	template <BlockAlgoAxis Axis, typename GetBlockFunc, typename GetLightFunc>
	inline void generate_regular_mesh_axis(GetBlockFunc &&get_block_func, GetLightFunc &&get_light_func) {
		using T = typename Config::Type;

		texture::BlockTexture texture_mask[2][Config::template GetArea<Axis, uint32_t>()];
		Light4 light_mask[2][Config::template GetArea<Axis, uint32_t>()];

		for (T xa = Config::template GetMin<Axis>(); xa <= Config::template GetMax<Axis>(); ++xa) {
			// A slice
			uint8_t face_mask = 0;
			{
				uint32_t counter = 0;
				Config::template Loop2<Axis>(xa, [&](T x, T y, T z) {
					T nx = Axis == 0 ? x - 1 : x, ny = Axis == 1 ? y - 1 : y, nz = Axis == 2 ? z - 1 : z;
					const auto get_tex = [this, &get_block_func](auto x, auto y, auto z, block::BlockFace face) {
						auto it = m_dynamic_block_textures.find(glm::vec<3, T>(x, y, z));
						return it == m_dynamic_block_textures.end() ? get_block_func(x, y, z).GetTexture(face)
						                                            : it->second[face];
					};
					constexpr block::BlockFace kFA = Axis << 1, kFB = kFA | 1;
					const texture::BlockTexture tex_a = get_tex(nx, ny, nz, kFA), tex_b = get_tex(x, y, z, kFB);

					if (xa != Config::template GetMin<Axis>() && tex_a.Show(tex_b)) {
						texture_mask[0][counter] = tex_a;
						light4_init(get_block_func, get_light_func, light_mask[0] + counter, kFA, nx, ny, nz);
						face_mask |= 1u;
					}
					if (xa != Config::template GetMax<Axis>() && tex_b.Show(tex_a)) {
						texture_mask[1][counter] = tex_b;
						light4_init(get_block_func, get_light_func, light_mask[1] + counter, kFB, x, y, z);
						face_mask |= 2u;
					}

					++counter;
				});
			}

			const auto push_mesh = [&](uint8_t quad_face_inv) {
				if (!(face_mask & (1u << quad_face_inv)))
					return;

				uint32_t counter = 0;

				auto local_texture_mask = texture_mask[quad_face_inv];
				auto local_light_mask = light_mask[quad_face_inv];

				constexpr BlockAlgoAxis kUAxis = Config::template GetNextAxis<Axis>();
				constexpr BlockAlgoAxis kVAxis = Config::template GetNextAxis2<Axis>();

				for (T u = Config::template GetMin<kUAxis>(); u < Config::template GetMax<kUAxis>(); ++u) {
					for (T v = Config::template GetMin<kVAxis>(); v < Config::template GetMax<kVAxis>();) {
						const texture::BlockTexture quad_texture = local_texture_mask[counter];
						if (!quad_texture.Empty()) {
							const Light4 quad_light = local_light_mask[counter];
							// Compute width
							uint32_t width, height;
							for (width = 1; quad_texture == local_texture_mask[counter + width] &&
							                quad_light == local_light_mask[counter + width] &&
							                v + width < Config::template GetMax<kVAxis>();
							     ++width)
								;

							// Compute height
							for (height = 1; u + height < Config::template GetMax<kUAxis>(); ++height)
								for (uint32_t k = 0; k < width; ++k) {
									uint32_t idx = counter + k + height * Config::template GetSpan<kVAxis, uint32_t>();
									if (quad_texture != local_texture_mask[idx] ||
									    quad_light != local_light_mask[idx]) {
										goto end_height_loop;
									}
								}
						end_height_loop:

							// Add quad
							auto [x, y, z] = Config::template ToXYZ<Axis, uint32_t>(xa, u, v);
							uint32_t ux, uy, uz, vx, vy, vz;
							if (quad_face_inv ^ ((kUAxis + 1) % 3 == kVAxis)) {
								std::tie(ux, uy, uz) = Config::template ToXYZ<Axis, uint32_t>(0, height, 0);
								std::tie(vx, vy, vz) = Config::template ToXYZ<Axis, uint32_t>(0, 0, width);
							} else {
								std::tie(ux, uy, uz) = Config::template ToXYZ<Axis, uint32_t>(0, 0, width);
								std::tie(vx, vy, vz) = Config::template ToXYZ<Axis, uint32_t>(0, height, 0);
							}

							// TODO: process resource rotation

							BlockMesh &info =
							    quad_texture.UseTransparentPass() ? m_transparent_mesh_info : m_opaque_mesh_info;
							// if indices would exceed, restart
							uint16_t cur_vertex = info.vertices.size();
							if (cur_vertex + 4 > UINT16_MAX) {
								bool trans = info.transparent;
								m_meshes.push_back(std::move(info));
								info = BlockMesh{};
								info.transparent = trans;
							}
							info.aabb.Merge({{uint32_t(x) << BlockVertex::kUnitBitOffset,
							                  uint32_t(y) << BlockVertex::kUnitBitOffset,
							                  uint32_t(z) << BlockVertex::kUnitBitOffset},
							                 {uint32_t(x + ux + vx) << BlockVertex::kUnitBitOffset,
							                  uint32_t(y + uy + vy) << BlockVertex::kUnitBitOffset,
							                  uint32_t(z + uz + vz) << BlockVertex::kUnitBitOffset}});

							block::BlockFace quad_face = (Axis << 1) | quad_face_inv;
							info.vertices.emplace_back(
							    x << BlockVertex::kUnitBitOffset, y << BlockVertex::kUnitBitOffset,
							    z << BlockVertex::kUnitBitOffset, Axis, quad_face, quad_light.ao[0],
							    quad_light.sunlight[0], quad_light.torchlight[0], quad_texture.GetID(),
							    quad_texture.GetTransformation());
							info.vertices.emplace_back(
							    (x + ux) << BlockVertex::kUnitBitOffset, (y + uy) << BlockVertex::kUnitBitOffset,
							    (z + uz) << BlockVertex::kUnitBitOffset, Axis, quad_face, quad_light.ao[1],
							    quad_light.sunlight[1], quad_light.torchlight[1], quad_texture.GetID(),
							    quad_texture.GetTransformation());
							info.vertices.emplace_back((x + ux + vx) << BlockVertex::kUnitBitOffset,
							                           (y + uy + vy) << BlockVertex::kUnitBitOffset,
							                           (z + uz + vz) << BlockVertex::kUnitBitOffset, Axis, quad_face,
							                           quad_light.ao[2], quad_light.sunlight[2],
							                           quad_light.torchlight[2], quad_texture.GetID(),
							                           quad_texture.GetTransformation());
							info.vertices.emplace_back(
							    (x + vx) << BlockVertex::kUnitBitOffset, (y + vy) << BlockVertex::kUnitBitOffset,
							    (z + vz) << BlockVertex::kUnitBitOffset, Axis, quad_face, quad_light.ao[3],
							    quad_light.sunlight[3], quad_light.torchlight[3], quad_texture.GetID(),
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

							for (uint32_t a = 0; a < height; ++a) {
								auto *base_ptr =
								    local_texture_mask + counter + Config::template GetSpan<kVAxis, uint32_t>() * a;
								std::fill(base_ptr, base_ptr + width, 0);
							}

							// Increase counters
							v += width;
							counter += width;
						} else {
							++v;
							++counter;
						}
					}
				}
			};

			push_mesh(0);
			push_mesh(1);
		}
	}

	std::vector<BlockMesh> m_meshes;
	BlockMesh m_opaque_mesh_info, m_transparent_mesh_info;

	block::Block m_neighbour_block_buffer[block::kBlockDynamicMeshMaxNeighbours];
	block::BlockMeshFace m_dynamic_block_mesh_buffer[block::kBlockMeshMaxFaceCount];
	std::unordered_map<glm::vec<3, typename Config::Type>, std::array<texture::BlockTexture, 6>>
	    m_dynamic_block_textures;
	std::array<texture::BlockTexture, 6> m_dynamic_block_texture_buffer;

public:
	template <typename GetBlockFunc, typename GetLightFunc>
	inline std::vector<BlockMesh> Generate(GetBlockFunc &&get_block_func, GetLightFunc &&get_light_func) {
		using T = typename Config::Type;
		static_assert(std::is_integral_v<T>);
		static_assert(Config::kBound.min_x >= 0 && Config::kBound.min_y >= 0 && Config::kBound.min_z >= 0);

		m_meshes.clear();
		m_dynamic_block_textures.clear();

		m_opaque_mesh_info = {};
		m_opaque_mesh_info.transparent = false;

		m_transparent_mesh_info = {};
		m_transparent_mesh_info.transparent = true;

		generate_custom_mesh(get_block_func, get_light_func);

		generate_regular_mesh_axis<0>(get_block_func, get_light_func);
		generate_regular_mesh_axis<1>(get_block_func, get_light_func);
		generate_regular_mesh_axis<2>(get_block_func, get_light_func);

		if (!m_opaque_mesh_info.vertices.empty())
			m_meshes.push_back(std::move(m_opaque_mesh_info));
		if (!m_transparent_mesh_info.vertices.empty())
			m_meshes.push_back(std::move(m_transparent_mesh_info));

		return std::move(m_meshes);
	}
};

} // namespace hc::client