#pragma once
#include "public/Trait.hpp"

template <> struct BlockTrait<kStone> : public SingleBlockTrait<kStone> {
	constexpr static BlockProperty kProperty = {
	    "Stone",
	    BLOCK_TEXTURE_SAME(BlockTextures::kStone),
	    BlockTransparencies::kOpaque,
	    BlockCollisionBits::kSolid,
	};
};
