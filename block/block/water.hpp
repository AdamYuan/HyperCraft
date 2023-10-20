#pragma once
#include "public/Trait.hpp"

template <> struct BlockTrait<kWater> : public SingleBlockTrait<kWater> {
	constexpr static BlockProperty kProperty = {
	    "Water",
	    BLOCK_TEXTURE_SAME(BlockTextures::kWater),
	    BlockTransparency::kSemiTransparent,
	    (LightLvl)0,
	    BlockCollisionBits::kLiquid,
	};
};
