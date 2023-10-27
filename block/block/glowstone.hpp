#pragma once
#include "public/Trait.hpp"

template <> struct BlockTrait<kGlowstone> : public SingleBlockTrait<kGlowstone> {
	constexpr static BlockProperty kProperty = {
	    "Glowstone",
	    BLOCK_TEXTURE_SAME(BlockTextures::kGlowstone),
	    BlockTransparency::kOpaque,
	    (LightLvl)15,
	    BlockCollision::kSolid,
	};
};
