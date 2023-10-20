#pragma once
#include "public/Trait.hpp"

template <> struct BlockTrait<kSandstone> : public SingleBlockTrait<kSandstone> {
	constexpr static BlockProperty kProperty = {
	    "Sandstone",
	    BLOCK_TEXTURE_SAME(BlockTextures::kSandstone),
	    BlockTransparency::kOpaque,
	    (LightLvl)0,
	    BlockCollisionBits::kSolid,
	};
};
