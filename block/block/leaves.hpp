#pragma once
#include "public/Trait.hpp"

template <> struct BlockTrait<kLeaves> : public MultiBlockTrait<kLeaves> {
	inline static constexpr BlockProperty kProperties[] = {
	    {"Oak Leaves", BLOCK_TEXTURE_SAME(BlockTextures::kOakLeaves), BlockTransparencies::kSemiTransparent,
	     BlockCollisionBits::kSolid},
	    {"Acacia Leaves", BLOCK_TEXTURE_SAME(BlockTextures::kAcaciaLeaves), BlockTransparencies::kSemiTransparent,
	     BlockCollisionBits::kSolid},
	    {"Jungle Leaves", BLOCK_TEXTURE_SAME(BlockTextures::kJungleLeaves), BlockTransparencies::kSemiTransparent,
	     BlockCollisionBits::kSolid},
	    {"Spruce Leaves", BLOCK_TEXTURE_SAME(BlockTextures::kSpruceLeaves), BlockTransparencies::kSemiTransparent,
	     BlockCollisionBits::kSolid},
	    {"Birch Leaves", BLOCK_TEXTURE_SAME(BlockTextures::kBirchLeaves), BlockTransparencies::kSemiTransparent,
	     BlockCollisionBits::kSolid},
	};
};
