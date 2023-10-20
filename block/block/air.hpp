#pragma once
#include "public/Trait.hpp"

template <> struct BlockTrait<kAir> : public SingleBlockTrait<kAir> {
	constexpr static BlockProperty kProperty = {"Air", BLOCK_TEXTURE_NONE, BlockTransparency::kTransparent, (LightLvl)0,
	                                            BlockCollisionBits::kNone};
};
