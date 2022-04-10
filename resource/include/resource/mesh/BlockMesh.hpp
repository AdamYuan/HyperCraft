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
	BlockFace face{std::numeric_limits<BlockFace>::max()};
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
	template <BlockTexID TexID, uint8_t Radius, uint8_t Height, typename = std::enable_if_t<Radius <= 8>>
	inline static constexpr BlockMesh kCross = {{
	                                                {BlockFaces::kFront,
	                                                 {TexID},
	                                                 {{8 - Radius, 0, 8 - Radius, 1},
	                                                  {8 + Radius, 0, 8 + Radius, 1},
	                                                  {8 + Radius, Height, 8 + Radius},
	                                                  {8 - Radius, Height, 8 - Radius}}},
	                                                {BlockFaces::kLeft,
	                                                 {TexID},
	                                                 {{8 - Radius, 0, 8 + Radius, 1},
	                                                  {8 + Radius, 0, 8 - Radius, 1},
	                                                  {8 + Radius, Height, 8 - Radius},
	                                                  {8 - Radius, Height, 8 + Radius}}},
	                                            },
	                                            2};
};

#endif
