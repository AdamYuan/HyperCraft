#pragma once
#include "public/Trait.hpp"

template <> struct BlockTrait<kIce> : public SingleBlockTrait<kIce> {
	constexpr static BlockProperty kProperty = {
	    "Ice",
	    BLOCK_TEXTURE_SAME(BlockTextures::kIce),
	    BlockTransparency::kSemiTransparent,
	    (LightLvl)0,
	    BlockCollision::kSolid,
	};
};
