#pragma once

#include <texture/BlockTexture.hpp>

#include "BlockEvent.hpp"
#include "BlockFace.hpp"
#include "BlockMesh.hpp"
#include "Light.hpp"

namespace hc::block {

using BlockID = uint8_t;
using BlockMeta = uint8_t;

enum class BlockTransparency : uint8_t { kOpaque, kSemiTransparent, kTransparent };
enum class BlockCollision : uint8_t { kNone, kSolid, kWater };
struct BlockProperty {
	const char *name{"Unnamed"};
	texture::BlockTexture textures[6]{};
	// Light Properties
	BlockTransparency transparency{BlockTransparency::kOpaque};
	LightLvl light_level{0};
	// Collision
	BlockCollision collision{BlockCollision::kSolid};
	// Mesh & Hitbox
	const BlockMesh *p_custom_mesh{nullptr};
	// Events
	const BlockEvent *p_event{nullptr};

	inline constexpr BlockProperty RotateCW(uint8_t axis) const {
		if (axis == 0) {
			return {name,
			        {
			            textures[0].RotateCW(),
			            textures[1].RotateCCW(),
			            textures[BlockFaces::kFront],
			            textures[BlockFaces::kBack],
			            textures[BlockFaces::kBottom],
			            textures[BlockFaces::kTop],
			        },
			        transparency,
			        light_level,
			        collision,
			        p_custom_mesh};
		} else if (axis == 1) {
			return {name,
			        {
			            textures[BlockFaces::kBack],
			            textures[BlockFaces::kFront],
			            textures[2].RotateCW(),
			            textures[3].RotateCCW(),
			            textures[BlockFaces::kRight],
			            textures[BlockFaces::kLeft],
			        },
			        transparency,
			        light_level,
			        collision,
			        p_custom_mesh};
		} else {
			return {name,
			        {
			            textures[BlockFaces::kTop],
			            textures[BlockFaces::kBottom],
			            textures[BlockFaces::kLeft].TransSwapUV(),
			            textures[BlockFaces::kRight].TransSwapUV(),
			            textures[4].RotateCW(),
			            textures[5].RotateCCW(),
			        },
			        transparency,
			        light_level,
			        collision,
			        p_custom_mesh};
		}
	}
};

} // namespace hc::block