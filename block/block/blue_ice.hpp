#pragma once
#include "public/Trait.hpp"

template <> struct BlockTrait<kBlueIce> : public SingleBlockTrait<kBlueIce> {
	constexpr static BlockProperty kProperty = {"Blue Ice", BLOCK_TEXTURE_SAME(BlockTextures::kBlueIce),
	                                            BlockTransparency::kOpaque, (LightLvl)0, BlockCollision::kSolid};
};
