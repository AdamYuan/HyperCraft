#pragma once
#include "public/Trait.hpp"

template <> struct BlockTrait<kSnow> : public SingleBlockTrait<kSnow> {
	constexpr static BlockProperty kProperty = {
	    "Snow",
	    BLOCK_TEXTURE_SAME(BlockTextures::kSnow),
	    BlockTransparency::kOpaque,
	    (LightLvl)0,
	    BlockCollisionBits::kSolid,
	};
};
