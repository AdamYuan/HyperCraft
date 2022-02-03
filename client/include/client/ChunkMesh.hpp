#ifndef CUBECRAFT3_COMMON_CHUNK_MESH_HPP
#define CUBECRAFT3_COMMON_CHUNK_MESH_HPP

#include <common/AABB.hpp>
#include <common/Block.hpp>
#include <common/Light.hpp>

struct ChunkMeshVertex { // Compressed mesh vertex for chunk
	// x, y, z, face, AO, sunlight, torchlight; resource, u, v
	uint32_t x5_y5_z5_face3_ao2_sl4_tl4, tex8_u5_v5;
	ChunkMeshVertex(uint8_t x, uint8_t y, uint8_t z, BlockFace face, uint8_t ao, LightLvl sunlight, LightLvl torchlight,
	                BlockTexID tex, uint8_t u, uint8_t v)
	    : x5_y5_z5_face3_ao2_sl4_tl4(x | (y << 5u) | (z << 10u) | (face << 15u) | (ao << 18u) | (sunlight << 20u) |
	                                 (torchlight << 24u)),
	      tex8_u5_v5((tex - 1u) | (u << 8u) | (v << 13u)) {}
};

struct ChunkMeshInfo {
	fAABB aabb;
	glm::i32vec3 base_position;
};
static_assert(sizeof(ChunkMeshInfo) == 36);

#endif
