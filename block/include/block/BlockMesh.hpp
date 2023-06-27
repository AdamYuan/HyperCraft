#pragma once

#include "BlockFace.hpp"
#include <AABB.hpp>
#include <texture/BlockTexture.hpp>

#include <cinttypes>
#include <limits>
#include <type_traits>

namespace hc::block {

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
	texture::BlockTexture texture{};
	BlockMeshVertex vertices[4]{};
};
constexpr uint32_t kBlockMeshMaxFaceCount = 32;
constexpr uint32_t kBlockMeshMaxAABBCount = 4;
struct BlockMesh {
	// "faces" array's BlockFace property should be sorted for better performance
	BlockMeshFace faces[kBlockMeshMaxFaceCount];
	uint32_t face_count{};
	u8AABB aabbs[kBlockMeshMaxAABBCount];
	uint32_t aabb_count{};
};

} // namespace hc::block