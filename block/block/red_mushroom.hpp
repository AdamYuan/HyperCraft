#pragma once
#include "public/CrossMesh.hpp"
#include "public/Trait.hpp"

template <> struct BlockTrait<kRedMushroom> : public SingleBlockTrait<kRedMushroom> {
	constexpr static BlockProperty kProperty = {
	    "Red Mushroom", BLOCK_TEXTURE_NONE,        BlockTransparency::kTransparent,
	    (LightLvl)0,    BlockCollisionBits::kNone, &kCrossMesh<BlockTextures::kRedMushroom, 5, 0, 12, true>,
	};
};
