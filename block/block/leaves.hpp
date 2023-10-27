#pragma once
#include "public/Trait.hpp"

template <> struct BlockTrait<kLeaves> : public MultiBlockTrait<kLeaves> {
	inline static constexpr BlockProperty kProperties[] = {
	    {"Oak Leaves", BLOCK_TEXTURE_SAME(BlockTextures::kOakLeaves), BlockTransparency::kSemiTransparent,
	     (LightLvl)0, BlockCollision::kSolid},
	    {"Acacia Leaves", BLOCK_TEXTURE_SAME(BlockTextures::kAcaciaLeaves), BlockTransparency::kSemiTransparent,
	     (LightLvl)0, BlockCollision::kSolid},
	    {"Jungle Leaves", BLOCK_TEXTURE_SAME(BlockTextures::kJungleLeaves), BlockTransparency::kSemiTransparent,
	     (LightLvl)0, BlockCollision::kSolid},
	    {"Spruce Leaves", BLOCK_TEXTURE_SAME(BlockTextures::kSpruceLeaves), BlockTransparency::kSemiTransparent,
	     (LightLvl)0, BlockCollision::kSolid},
	    {"Birch Leaves", BLOCK_TEXTURE_SAME(BlockTextures::kBirchLeaves), BlockTransparency::kSemiTransparent,
	     (LightLvl)0, BlockCollision::kSolid},
	};
};
