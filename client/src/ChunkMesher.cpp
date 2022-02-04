#include <client/ChunkMesher.hpp>

#include <client/WorldRenderer.hpp>

#include <bitset>
#include <spdlog/spdlog.h>

#include <glm/gtx/string_cast.hpp>

void ChunkMesher::generate_face_lights(
    ChunkMesher::Light4 face_lights[Chunk::kSize * Chunk::kSize * Chunk::kSize][6]) const {

	Block neighbour_blocks[27];
	Light neighbour_lights[27];
	for (uint32_t index = 0; index < Chunk::kSize * Chunk::kSize * Chunk::kSize; ++index) {
		int_fast8_t pos[3];
		Chunk::Index2XYZ(index, pos);

		Block block;
		if ((block = m_chunk_ptr->GetBlock(index)).GetID() == Blocks::kAir)
			continue;

		bool filled_neighbours = false;

		for (BlockFace face = 0; face < 6; ++face) {
			int_fast8_t nei[3] = {pos[0], pos[1], pos[2]};
			BlockFaceProceed(nei, face);

			Block nei_block = get_block(nei[0], nei[1], nei[2]);

			if (!block.ShowFace(nei_block))
				continue;

			if (!filled_neighbours) {
				filled_neighbours = true;
				uint32_t nei_idx = 0;
				for (auto x = (int_fast8_t)(pos[0] - 1); x <= pos[0] + 1; ++x)
					for (auto y = (int_fast8_t)(pos[1] - 1); y <= pos[1] + 1; ++y)
						for (auto z = (int_fast8_t)(pos[2] - 1); z <= pos[2] + 1; ++z, ++nei_idx) {
							neighbour_blocks[nei_idx] = get_block(x, y, z);
							neighbour_lights[nei_idx] = get_light(x, y, z);
						}
			}
			face_lights[index][face].Initialize(face, neighbour_blocks, neighbour_lights);
			// spdlog::info("face={}, (x, y, z)=({}, {}, {})", face, pos[0], pos[1], pos[2]);
		}
	}
}

AABB<uint_fast8_t> ChunkMesher::generate_mesh(const Light4 face_lights[Chunk::kSize * Chunk::kSize * Chunk::kSize][6],
                                              std::vector<ChunkMeshVertex> *vertices,
                                              std::vector<uint16_t> *indices) const {
	AABB<uint_fast8_t> aabb;

	uint32_t cur_vertex = 0;

	for (uint_fast8_t axis = 0; axis < 3; ++axis) {
		const uint_fast8_t u = (axis + 1) % 3;
		const uint_fast8_t v = (axis + 2) % 3;

		uint_fast8_t x[3]{0};
		int_fast8_t q[3]{0};
		BlockTexture texture_mask[Chunk::kSize * Chunk::kSize]{};
		std::bitset<Chunk::kSize * Chunk::kSize> face_inv_mask{};
		Light4 light_mask[Chunk::kSize * Chunk::kSize]{};

		// Compute texture_mask
		q[axis] = -1;
		for (x[axis] = 0; x[axis] <= Chunk::kSize; ++x[axis]) {
			uint32_t counter = 0;
			for (x[v] = 0; x[v] < Chunk::kSize; ++x[v]) {
				for (x[u] = 0; x[u] < Chunk::kSize; ++x[u], ++counter) {
					const Block a = get_block(x[0] + q[0], x[1] + q[1], x[2] + q[2]);
					const Block b = get_block(x[0], x[1], x[2]);

					if (x[axis] != 0 && a.ShowFace(b)) {
						BlockFace f = axis << 1;
						texture_mask[counter] = a.GetTexture(f);
						face_inv_mask[counter] = false;
						light_mask[counter] = face_lights[Chunk::XYZ2Index(x[0] + q[0], x[1] + q[1], x[2] + q[2])][f];
						// spdlog::info("axis = {}, face={}, ao={}, (x, y, z)=({}, {}, {})", axis, f,
						//             light_mask[counter].m_ao.m_data, x[0] + q[0], x[1] + q[1], x[2] + q[2]);
					} else if (Chunk::kSize != x[axis] && b.ShowFace(a)) {
						BlockFace f = (axis << 1) | 1;
						texture_mask[counter] = b.GetTexture(f);
						face_inv_mask[counter] = true;
						light_mask[counter] = face_lights[Chunk::XYZ2Index(x[0], x[1], x[2])][f];
						// spdlog::info("axis = {}, inv face={}, ao={}, (x, y, z)=({}, {}, {})", axis, f,
						//               light_mask[counter].m_ao.m_data, x[0], x[1], x[2]);
					} else
						texture_mask[counter] = 0;
				}
			}

			// ++x[axis];

			// Generate mesh for texture_mask using lexicographic ordering
			counter = 0;
			for (uint_fast8_t j = 0; j < Chunk::kSize; ++j) {
				for (uint_fast8_t i = 0; i < Chunk::kSize;) {
					const BlockTexture quad_texture = texture_mask[counter];
					if (!quad_texture.Empty()) {
						const bool quad_face_inv = face_inv_mask[counter];
						const Light4 quad_light = light_mask[counter];
						// Compute width
						uint_fast8_t width, height;
						for (width = 1; quad_texture == texture_mask[counter + width] &&
						                quad_face_inv == face_inv_mask[counter + width] &&
						                quad_light == light_mask[counter + width] && i + width < Chunk::kSize;
						     ++width)
							;

						// Compute height
						for (height = 1; j + height < Chunk::kSize; ++height)
							for (uint_fast8_t k = 0; k < width; ++k) {
								uint32_t idx = counter + k + height * Chunk::kSize;
								if (quad_texture != texture_mask[idx] || quad_face_inv != face_inv_mask[idx] ||
								    quad_light != light_mask[idx]) {
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

						uint8_t                   // resource uvs
						    v00u = du[u] + dv[u], //
						    v00v = du[v] + dv[v], // v00: u, v
						    v01u = dv[u],         //
						    v01v = dv[v],         // v01: u, v
						    v10u = 0,             //
						    v10v = 0,             // v10: u, v
						    v11u = du[u],         //
						    v11v = du[v];         // v11: u, v

						// TODO: process resource rotation
						// if (quad_texture.GetRotation() == )

						// face specified UV transforms
						BlockFace quad_face = ((axis << 1) | quad_face_inv);
						if (quad_face == BlockFaces::kLeft) {
							std::swap(v00u, v01v);
							std::swap(v00v, v01u);
							std::swap(v11u, v10v);
							std::swap(v11v, v10u);
						} else if (quad_face == BlockFaces::kRight) {
							std::swap(v11u, v11v);
							std::swap(v01u, v01v);
							std::swap(v00u, v00v);
						} else if (quad_face == BlockFaces::kFront) {
							std::swap(v00u, v01u);
							std::swap(v00v, v01v);
							std::swap(v11u, v10u);
							std::swap(v11v, v10v);
						}

						ChunkMeshVertex v00{uint8_t(x[0]),
						                    uint8_t(x[1]),
						                    uint8_t(x[2]),
						                    quad_face,
						                    quad_light.m_ao[0],
						                    quad_light.m_light[0].GetSunlight(),
						                    quad_light.m_light[0].GetTorchlight(),
						                    quad_texture.GetID(),
						                    v00u,
						                    v00v},
						    v01 = {uint8_t(x[0] + du[0]),
						           uint8_t(x[1] + du[1]),
						           uint8_t(x[2] + du[2]),
						           quad_face,
						           quad_light.m_ao[1],
						           quad_light.m_light[1].GetSunlight(),
						           quad_light.m_light[1].GetTorchlight(),
						           quad_texture.GetID(),
						           v01u,
						           v01v},
						    v10 = {uint8_t(x[0] + du[0] + dv[0]),
						           uint8_t(x[1] + du[1] + dv[1]),
						           uint8_t(x[2] + du[2] + dv[2]),
						           quad_face,
						           quad_light.m_ao[2],
						           quad_light.m_light[2].GetSunlight(),
						           quad_light.m_light[2].GetTorchlight(),
						           quad_texture.GetID(),
						           v10u,
						           v10v},
						    v11 = {uint8_t(x[0] + dv[0]),
						           uint8_t(x[1] + dv[1]),
						           uint8_t(x[2] + dv[2]),
						           quad_face,
						           quad_light.m_ao[3],
						           quad_light.m_light[3].GetSunlight(),
						           quad_light.m_light[3].GetTorchlight(),
						           quad_texture.GetID(),
						           v11u,
						           v11v};
						aabb.Merge(
						    {{x[0], x[1], x[2]}, {x[0] + du[0] + dv[0], x[1] + du[1] + dv[1], x[2] + du[2] + dv[2]}});

						vertices->push_back(v00);
						vertices->push_back(v01);
						vertices->push_back(v10);
						vertices->push_back(v11);

						if (quad_light.GetFlip()) {
							// 11--------10
							//|       / |
							//|    /    |
							//| /       |
							// 00--------01
							indices->push_back(cur_vertex);
							indices->push_back(cur_vertex + 1);
							indices->push_back(cur_vertex + 2);

							indices->push_back(cur_vertex);
							indices->push_back(cur_vertex + 2);
							indices->push_back(cur_vertex + 3);
						} else {
							// 11--------10
							//| \       |
							//|    \    |
							//|       \ |
							// 00--------01
							indices->push_back(cur_vertex + 1);
							indices->push_back(cur_vertex + 2);
							indices->push_back(cur_vertex + 3);

							indices->push_back(cur_vertex);
							indices->push_back(cur_vertex + 1);
							indices->push_back(cur_vertex + 3);
						}
						cur_vertex += 4;

						for (uint_fast8_t b = 0; b < width; ++b)
							for (uint_fast8_t a = 0; a < height; ++a)
								texture_mask[counter + b + a * Chunk::kSize] = 0;

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
	return aabb;
}

void ChunkMesher::Light4::Initialize(BlockFace face, const Block neighbour_blocks[27],
                                     const Light neighbour_lights[27]) {
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

	constexpr uint32_t kLookup3[6][4][3] = {
	    {{21, 18, 19}, {21, 24, 25}, {23, 26, 25}, {23, 20, 19}}, {{3, 0, 1}, {5, 2, 1}, {5, 8, 7}, {3, 6, 7}},
	    {{15, 6, 7}, {17, 8, 7}, {17, 26, 25}, {15, 24, 25}},     {{9, 0, 1}, {9, 18, 19}, {11, 20, 19}, {11, 2, 1}},
	    {{11, 2, 5}, {11, 20, 23}, {17, 26, 23}, {17, 8, 5}},     {{9, 0, 3}, {15, 6, 3}, {15, 24, 21}, {9, 18, 21}}};
	constexpr uint32_t kLookup1[6] = {22, 4, 16, 10, 14, 12};

	Block sides[3];
	bool trans[3], pass[3];

	for (uint32_t v = 0; v < 4; ++v) {
		for (uint32_t i = 0; i < 3; ++i) {
			sides[i] = neighbour_blocks[kLookup3[face][v][i]];
			trans[i] = sides[i].GetTransparent();
			pass[i] = sides[i].GetLightPass();
		}

		m_ao.Set(v, (!trans[0] && !trans[2] ? 0u : 3u - !trans[0] - !trans[1] - !trans[2]));

		// smooth the LightLvl using the average value
		uint32_t counter = 1, sunlight_sum = neighbour_lights[kLookup1[face]].GetSunlight(),
		         torchlight_sum = neighbour_lights[kLookup1[face]].GetTorchlight();
		if (pass[0] || pass[2])
			for (uint32_t i = 0; i < 3; ++i) {
				if (!pass[i])
					continue;
				counter++;
				sunlight_sum += neighbour_lights[kLookup3[face][v][i]].GetSunlight();
				torchlight_sum += neighbour_lights[kLookup3[face][v][i]].GetTorchlight();
			}
		m_light[v].SetSunlight(sunlight_sum / counter);
		m_light[v].SetTorchlight(torchlight_sum / counter);
		// spdlog::info("face={} torchlight={} sunlight={} ao={}", face, m_light[v].GetTorchlight(),
		//                m_light[v].GetSunlight(), m_ao[v]);
	}
}

#include <random>
void ChunkMesher::Run() {
	if (!lock()) {
		return;
	}

	// if the neighbour chunks are not totally generated, return and move it back
	if (!m_chunk_ptr->HaveFlags(Chunk::Flag::kGenerated)) {
		push_worker(ChunkMesher::Create(m_chunk_ptr));
		spdlog::info("remesh");
		return;
	}
	for (const auto &i : m_neighbour_chunk_ptr)
		if (!i->HaveFlags(Chunk::Flag::kGenerated)) {
			push_worker(ChunkMesher::Create(m_chunk_ptr));
			spdlog::info("remesh");
			return;
		}

	std::vector<ChunkMeshVertex> vertices;
	std::vector<uint16_t> indices;
	AABB<uint_fast8_t> aabb;
	{
		Light4 face_lights[Chunk::kSize * Chunk::kSize * Chunk::kSize][6];
		generate_face_lights(face_lights);
		aabb = generate_mesh(face_lights, &vertices, &indices);
	}
	spdlog::info("Chunk {} meshed with {} vertices and {} indices, aabb: ({}, {})",
	             glm::to_string(m_chunk_ptr->GetPosition()), vertices.size(), indices.size(),
	             glm::to_string(aabb.GetMin()), glm::to_string(aabb.GetMax()));

	std::shared_ptr<World> world_ptr = m_chunk_ptr->LockWorld();
	if (!world_ptr)
		return;
	std::shared_ptr<WorldRenderer> world_renderer_ptr = world_ptr->LockWorldRenderer();
	if (!world_renderer_ptr)
		return;

	glm::i32vec3 base_position = (glm::i32vec3)m_chunk_ptr->GetPosition() * (int32_t)Chunk::kSize;
	m_chunk_ptr->m_mesh_handle = world_renderer_ptr->GetChunkRenderer()->PushMesh(
	    vertices, indices, {(fAABB)((i32AABB)aabb + base_position), base_position});
	m_chunk_ptr->EnableFlags(Chunk::Flag::kMeshed);
}
