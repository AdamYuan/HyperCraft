#ifndef CUBECRAFT3_COMMON_CHUNK_MESH_HPP
#define CUBECRAFT3_COMMON_CHUNK_MESH_HPP

#include <common/AABB.hpp>
#include <common/Block.hpp>
#include <common/Light.hpp>

#include <client/MeshEraser.hpp>
#include <client/MeshHandle.hpp>
#include <client/MeshRendererBase.hpp>

struct ChunkMeshVertex { // Compressed mesh vertex for chunk
	static constexpr uint32_t kUnitBitOffset = 4u;
	static constexpr uint32_t kUnitOffset = 1u << kUnitBitOffset;
	// x, y, z, face, AO, sunlight, torchlight; resource, u, v
	uint32_t x10_y10_z10, tex8_face3_ao2_sl6_tl6;
	ChunkMeshVertex(uint32_t x10, uint32_t y10, uint32_t z10, BlockFace face, uint8_t ao, uint8_t sunlight,
	                uint8_t torchlight, BlockTexID tex)
	    : x10_y10_z10(x10 | (y10 << 10u) | (z10 << 20u)),
	      tex8_face3_ao2_sl6_tl6((tex - 1u) | (face << 8u) | (ao << 11u) | (sunlight << 13u) | (torchlight << 19u)) {}
};

struct ChunkMeshInfo {
	fAABB aabb;
	glm::i32vec3 base_position;
	uint32_t transparent;
};
static_assert(sizeof(ChunkMeshInfo) == 40);

using ChunkMeshHandle = MeshHandle<ChunkMeshVertex, uint16_t, ChunkMeshInfo, 2>;
using ChunkMeshCluster = MeshCluster<ChunkMeshVertex, uint16_t, ChunkMeshInfo, 2>;
using ChunkMeshRendererBase = MeshRendererBase<ChunkMeshVertex, uint16_t, ChunkMeshInfo, 2>;
using ChunkMeshEraser = MeshEraser<ChunkMeshVertex, uint16_t, ChunkMeshInfo, 2>;

#endif
