#pragma once
#include "public/Trait.hpp"

template <> struct BlockTrait<kSandstone> : public SingleBlockTrait<kSandstone> {
	constexpr static BlockProperty kProperty = {
	    "Sandstone",
	    BLOCK_TEXTURE_SAME(BlockTextures::kSandstone),
	    BlockTransparencies::kOpaque,
	    BlockCollisionBits::kSolid,
	};
};
