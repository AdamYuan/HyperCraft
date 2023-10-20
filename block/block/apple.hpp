#pragma once
#include "public/CrossMesh.hpp"
#include "public/Trait.hpp"

template <> struct BlockTrait<kApple> : public SingleBlockTrait<kApple> {
	constexpr static BlockProperty kProperty = {
	    "Apple",     BLOCK_TEXTURE_NONE,        BlockTransparency::kTransparent,
	    (LightLvl)0, BlockCollisionBits::kNone, &kCrossMesh<BlockTextures::kApple, 5, 1, 15, BlockFaces::kBottom>,
	};
};
