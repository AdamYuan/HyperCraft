#ifndef CUBECRAFT3_RESOURCE_BLOCK_MESH_HPP
#define CUBECRAFT3_RESOURCE_BLOCK_MESH_HPP

#include <common/AABB.hpp>
#include <common/BlockFace.hpp>

#include <resource/texture/BlockTexture.hpp>

#include <cinttypes>
#include <limits>
#include <type_traits>

struct BlockMeshVertex {
	union {
		struct {
			uint8_t x{}, y{}, z{};
		};
		uint8_t pos[3];
	};
	uint8_t ao{4}; // 4 means auto AO
	constexpr BlockMeshVertex() {}
	template <typename T, typename = std::enable_if_t<std::is_integral_v<T>>>
	constexpr BlockMeshVertex(T x, T y, T z, T ao = 4) : x(x), y(y), z(z), ao(ao) {}
};
struct BlockMeshFace {
	uint8_t axis;
	BlockFace light_face{}, render_face{};
	BlockTexture texture{};
	BlockMeshVertex vertices[4]{};
};
#define BLOCK_MESH_MAX_FACE_COUNT 32
constexpr uint32_t kBlockMeshMaxHitboxCount = 4;
struct BlockMesh {
	// "faces" array's BlockFace property should be sorted for better performance
	BlockMeshFace faces[BLOCK_MESH_MAX_FACE_COUNT];
	uint32_t face_count{};
	u8AABB hitboxes[kBlockMeshMaxHitboxCount];
	uint32_t hitbox_count{};
};
#undef BLOCK_MESH_MAX_FACE_COUNT

// predefined block meshes
struct BlockMeshes {
	inline static constexpr BlockMesh CactusSides() {
		return {{
		            {0,
		             BlockFaces::kRight,
		             BlockFaces::kRight,
		             {BlockTextures::kCactusSide},
		             {
		                 {15, 0, 16},
		                 {15, 0, 0},
		                 {15, 16, 0},
		                 {15, 16, 16},
		             }},
		            {0,
		             BlockFaces::kLeft,
		             BlockFaces::kLeft,
		             {BlockTextures::kCactusSide},
		             {
		                 {1, 0, 0},
		                 {1, 0, 16},
		                 {1, 16, 16},
		                 {1, 16, 0},
		             }},
		            {2,
		             BlockFaces::kFront,
		             BlockFaces::kFront,
		             {BlockTextures::kCactusSide},
		             {
		                 {0, 0, 15},
		                 {16, 0, 15},
		                 {16, 16, 15},
		                 {0, 16, 15},
		             }},
		            {2,
		             BlockFaces::kBack,
		             BlockFaces::kBack,
		             {BlockTextures::kCactusSide},
		             {
		                 {16, 0, 1},
		                 {0, 0, 1},
		                 {0, 16, 1},
		                 {16, 16, 1},
		             }},
		        },
		        4,
		        {{{1, 0, 1}, {15, 16, 15}}},
		        1};
	}
	inline static constexpr BlockMesh Cross(BlockTexID tex_id, int radius, int low, int high,
	                                        BlockFace light_face = BlockFaces::kTop) {
		return {{
		            {0,
		             light_face,
		             BlockFaces::kLeft,
		             {tex_id},
		             {
		                 {8 - radius, low, 8 - radius},
		                 {8 + radius, low, 8 + radius},
		                 {8 + radius, high, 8 + radius},
		                 {8 - radius, high, 8 - radius},
		             }},
		            {2,
		             light_face,
		             BlockFaces::kBack,
		             {tex_id, BlockTextures::kTransNegU},
		             {
		                 {8 - radius, low, 8 + radius},
		                 {8 + radius, low, 8 - radius},
		                 {8 + radius, high, 8 - radius},
		                 {8 - radius, high, 8 + radius},
		             }},
		            {0,
		             light_face,
		             BlockFaces::kRight,
		             {tex_id},
		             {
		                 {8 - radius, high, 8 - radius},
		                 {8 + radius, high, 8 + radius},
		                 {8 + radius, low, 8 + radius},
		                 {8 - radius, low, 8 - radius},
		             }},
		            {2,
		             light_face,
		             BlockFaces::kFront,
		             {tex_id, BlockTextures::kTransNegU},
		             {
		                 {8 - radius, high, 8 + radius},
		                 {8 + radius, high, 8 - radius},
		                 {8 + radius, low, 8 - radius},
		                 {8 - radius, low, 8 + radius},
		             }},
		        },
		        BlockTexture{tex_id}.UseTransparentPass() ? 2u : 4u,
		        {{{8 - radius, low, 8 - radius}, {8 + radius, high, 8 + radius}}},
		        1};
	}

	inline static constexpr BlockMesh InnerSurface(BlockTexID tex_id, BlockFace face, uint8_t dist = 1) {
		uint8_t axis = face >> 1, u = (axis + 1) % 3, v = (axis + 2) % 3;
		uint8_t du[3] = {0}, dv[3] = {0}, x[3] = {};
		if (face & 1u) {
			du[v] = dv[u] = 16;
			x[axis] = 16 - (int32_t)dist;
		} else {
			dv[v] = du[u] = 16;
			x[axis] = dist;
		}

		return {{
		            {axis,
		             face,
		             face,
		             {tex_id},
		             {
		                 {x[0], x[1], x[2]},
		                 {x[0] + du[0], x[1] + du[1], x[2] + du[2]},
		                 {x[0] + du[0] + dv[0], x[1] + du[1] + dv[1], x[2] + du[2] + dv[2]},
		                 {x[0] + dv[0], x[1] + dv[1], x[2] + dv[2]},
		             }},
		            {axis,
		             face,
		             (BlockFace)(face ^ 1u),
		             {tex_id},
		             {
		                 {x[0], x[1], x[2]},
		                 {x[0] + dv[0], x[1] + dv[1], x[2] + dv[2]},
		                 {x[0] + du[0] + dv[0], x[1] + du[1] + dv[1], x[2] + du[2] + dv[2]},
		                 {x[0] + du[0], x[1] + du[1], x[2] + du[2]},
		             }},
		        },
		        BlockTexture{tex_id}.UseTransparentPass() ? 1u : 2u,
		        {{{x[0], x[1], x[2]}, {x[0] + du[0] + dv[0], x[1] + du[1] + dv[1], x[2] + du[2] + dv[2]}}},
		        1u};
	}
};

#endif
