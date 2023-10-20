#pragma once
#include "public/Trait.hpp"

template <> struct BlockTrait<kDirt> : public SingleBlockTrait<kDirt> {
	constexpr static BlockProperty kProperty = {
	    "Dirt",
	    BLOCK_TEXTURE_SAME(BlockTextures::kDirt),
	    BlockTransparency::kOpaque,
	    (LightLvl)0,
	    BlockCollisionBits::kSolid,
	};
};
