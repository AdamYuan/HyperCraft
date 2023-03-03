#ifndef CUBECRAFT3_COMMON_CHUNK_MESH_HPP
#define CUBECRAFT3_COMMON_CHUNK_MESH_HPP

#include <common/AABB.hpp>
#include <common/Block.hpp>
#include <common/Light.hpp>

#include <client/MeshEraser.hpp>
#include <client/MeshHandle.hpp>
#include <client/MeshPool.hpp>

struct ChunkMeshVertex { // Compressed mesh vertex for chunk
	static constexpr uint32_t kUnitBitOffset = 4u;
	static constexpr uint32_t kUnitOffset = 1u << kUnitBitOffset;
	// x, y, z, axis, texture id, texture transformation, face, AO, sunlight, torchlight
	uint32_t x10_y10_z10_axis2, tex10_trans3_face3_ao2_sl6_tl6;
	ChunkMeshVertex(uint32_t x10, uint32_t y10, uint32_t z10, uint8_t axis, BlockFace face, uint8_t ao,
	                uint8_t sunlight, uint8_t torchlight, BlockTexID tex, BlockTexTrans tex_trans)
	    : x10_y10_z10_axis2(x10 | (y10 << 10u) | (z10 << 20u) | (axis << 30u)),
	      tex10_trans3_face3_ao2_sl6_tl6((tex - 1u) | (tex_trans << 10u) | (face << 13u) | (ao << 16u) |
	                                     (sunlight << 18u) | (torchlight << 24u)) {}
};

struct ChunkMeshInfo {
	fAABB aabb;
	glm::i32vec3 base_position;
	uint32_t transparent;
};
static_assert(sizeof(ChunkMeshInfo) == 40);

using ChunkMeshHandle = MeshHandle<ChunkMeshVertex, uint16_t, ChunkMeshInfo>;
using ChunkMeshCluster = MeshCluster<ChunkMeshVertex, uint16_t, ChunkMeshInfo>;
using ChunkMeshPoolBase = MeshPool<ChunkMeshVertex, uint16_t, ChunkMeshInfo>;
using ChunkMeshEraser = MeshEraser<ChunkMeshVertex, uint16_t, ChunkMeshInfo>;

#endif
