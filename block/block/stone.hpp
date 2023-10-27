#pragma once
#include "public/Trait.hpp"

template <> struct BlockTrait<kStone> : public SingleBlockTrait<kStone> {
	constexpr static BlockProperty kProperty = {
	    "Stone",
	    BLOCK_TEXTURE_SAME(BlockTextures::kStone),
	    BlockTransparency::kOpaque,
	    (LightLvl)0,
	    BlockCollision::kSolid,
	};
};
