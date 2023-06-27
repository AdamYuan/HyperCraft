#pragma once
#include "public/Trait.hpp"

template <> struct BlockTrait<kPlank> : public MultiBlockTrait<kPlank> {
	inline static constexpr BlockProperty kProperties[] = {
	    {
	        "Oak Plank",
	        BLOCK_TEXTURE_SAME(BlockTextures::kOakPlank),
	        BlockTransparencies::kOpaque,
	        BlockCollisionBits::kSolid,
	    },
	    {
	        "Acacia Plank",
	        BLOCK_TEXTURE_SAME(BlockTextures::kAcaciaPlank),
	        BlockTransparencies::kOpaque,
	        BlockCollisionBits::kSolid,
	    },
	    {
	        "Jungle Plank",
	        BLOCK_TEXTURE_SAME(BlockTextures::kJunglePlank),
	        BlockTransparencies::kOpaque,
	        BlockCollisionBits::kSolid,
	    },
	    {
	        "Spruce Plank",
	        BLOCK_TEXTURE_SAME(BlockTextures::kSprucePlank),
	        BlockTransparencies::kOpaque,
	        BlockCollisionBits::kSolid,
	    },
	    {
	        "Birch Plank",
	        BLOCK_TEXTURE_SAME(BlockTextures::kBirchPlank),
	        BlockTransparencies::kOpaque,
	        BlockCollisionBits::kSolid,
	    },
	};
};
