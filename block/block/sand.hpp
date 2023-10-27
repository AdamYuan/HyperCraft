#pragma once
#include "public/Trait.hpp"

template <> struct BlockTrait<kSand> : public SingleBlockTrait<kSand> {
	constexpr static BlockProperty kProperty = {
	    "Sand",
	    BLOCK_TEXTURE_SAME(BlockTextures::kSand),
	    BlockTransparency::kOpaque,
	    (LightLvl)0,
	    BlockCollision::kSolid,
	};
};
