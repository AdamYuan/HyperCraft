#pragma once
#include "public/Trait.hpp"

template <> struct BlockTrait<kFlowingWater> : public SingleBlockTrait<kFlowingWater> {
	constexpr static BlockProperty kProperty = {
	    "Flowing Water",
	    BLOCK_TEXTURE_SAME(BlockTextures::kWater),
	    BlockTransparency::kSemiTransparent,
	    (LightLvl)0,
	    BlockCollision::kWater,
	};
};
