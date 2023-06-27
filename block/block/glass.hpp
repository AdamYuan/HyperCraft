#pragma once
#include "public/Trait.hpp"

template <> struct BlockTrait<kGlass> : public SingleBlockTrait<kGlass> {
	constexpr static BlockProperty kProperty = {
	    "Glass",
	    BLOCK_TEXTURE_SAME(BlockTextures::kGlass),
	    BlockTransparencies::kTransparent,
	    BlockCollisionBits::kSolid,
	};
};
