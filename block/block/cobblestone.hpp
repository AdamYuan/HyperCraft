#pragma once
#include "public/Trait.hpp"

template <> struct BlockTrait<kCobblestone> : public SingleBlockTrait<kCobblestone> {
	constexpr static BlockProperty kProperty = {
	    "Cobblestone",
	    BLOCK_TEXTURE_SAME(BlockTextures::kCobblestone),
	    BlockTransparency::kOpaque,
	    (LightLvl)0,
	    BlockCollisionBits::kSolid,
	};
};
