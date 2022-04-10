#ifndef CUBECRAFT3_RESOURCE_BLOCK_MESH_HPP
#define CUBECRAFT3_RESOURCE_BLOCK_MESH_HPP

#include <resource/texture/BlockTexture.hpp>

#include <cinttypes>
#include <limits>
#include <type_traits>

using BlockFace = uint8_t;
struct BlockFaces {
	enum FACE : BlockFace { kRight = 0, kLeft, kTop, kBottom, kFront, kBack };
};

struct BlockMeshVertex {
	union {
		struct {
			uint8_t x{}, y{}, z{};
		};
		uint8_t pos[3];
	};
	uint8_t ao{3};
};
struct BlockMeshFace {
	uint8_t axis;
	BlockFace light_face{}, render_face{};
	BlockTexture texture{};
	BlockMeshVertex vertices[4]{};
};
#define BLOCK_MESH_MAX_FACE_COUNT 32
struct BlockMesh {
	// "faces" array's BlockFace property should be sorted for better performance
	BlockMeshFace faces[BLOCK_MESH_MAX_FACE_COUNT];
	uint32_t face_count{};
};
#undef BLOCK_MESH_MAX_FACE_COUNT

// predefined block meshes
struct BlockMeshes {
	template <BlockTexID TexID, uint8_t Radius, uint8_t Low, uint8_t High, bool DoubleSide = false,
	          BlockFace LightFace = BlockFaces::kTop, typename = std::enable_if_t<Radius <= 8>>
	inline static constexpr BlockMesh kCross = {{
	                                                {0,
	                                                 LightFace,
	                                                 BlockFaces::kLeft,
	                                                 {TexID},
	                                                 {
	                                                     {8 - Radius, Low, 8 - Radius, 1},
	                                                     {8 + Radius, Low, 8 + Radius, 1},
	                                                     {8 + Radius, High, 8 + Radius},
	                                                     {8 - Radius, High, 8 - Radius},
	                                                 }},
	                                                {2,
	                                                 LightFace,
	                                                 BlockFaces::kBack,
	                                                 {TexID},
	                                                 {
	                                                     {8 - Radius, Low, 8 + Radius, 1},
	                                                     {8 + Radius, Low, 8 - Radius, 1},
	                                                     {8 + Radius, High, 8 - Radius},
	                                                     {8 - Radius, High, 8 + Radius},
	                                                 }},
	                                                {0,
	                                                 LightFace,
	                                                 BlockFaces::kRight,
	                                                 {TexID},
	                                                 {
	                                                     {8 - Radius, High, 8 - Radius},
	                                                     {8 + Radius, High, 8 + Radius},
	                                                     {8 + Radius, Low, 8 + Radius, 1},
	                                                     {8 - Radius, Low, 8 - Radius, 1},
	                                                 }},
	                                                {2,
	                                                 LightFace,
	                                                 BlockFaces::kFront,
	                                                 {TexID},
	                                                 {
	                                                     {8 - Radius, High, 8 + Radius},
	                                                     {8 + Radius, High, 8 - Radius},
	                                                     {8 + Radius, Low, 8 - Radius, 1},
	                                                     {8 - Radius, Low, 8 + Radius, 1},
	                                                 }},
	                                            },
	                                            DoubleSide ? 4 : 2};
};

#endif
