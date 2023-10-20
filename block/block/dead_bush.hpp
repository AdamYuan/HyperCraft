#pragma once
#include "public/CrossMesh.hpp"
#include "public/Trait.hpp"

template <> struct BlockTrait<kDeadBush> : public SingleBlockTrait<kDeadBush> {
	constexpr static BlockProperty kProperty = {
	    "Dead Bush", BLOCK_TEXTURE_NONE,        BlockTransparency::kTransparent,
	    (LightLvl)0, BlockCollisionBits::kNone, &kCrossMesh<BlockTextures::kDeadBush, 8, 0, 16>,
	};
};
