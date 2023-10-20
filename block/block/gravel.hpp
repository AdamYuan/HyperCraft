#pragma once
#include "public/Trait.hpp"

template <> struct BlockTrait<kGravel> : public SingleBlockTrait<kGravel> {
	constexpr static BlockProperty kProperty = {
	    "Gravel",
	    BLOCK_TEXTURE_SAME(BlockTextures::kGravel),
	    BlockTransparency::kOpaque,
	    (LightLvl)0,
	    BlockCollisionBits::kSolid,
	};
};
