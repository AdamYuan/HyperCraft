#pragma once
#include "public/Trait.hpp"

template <> struct BlockTrait<kPlank> : public MultiBlockTrait<kPlank> {
	inline static constexpr BlockProperty kProperties[] = {
	    {
	        "Oak Plank",
	        BLOCK_TEXTURE_SAME(BlockTextures::kOakPlank),
	        BlockTransparency::kOpaque,
	        (LightLvl)0,
	        BlockCollision::kSolid,
	    },
	    {
	        "Acacia Plank",
	        BLOCK_TEXTURE_SAME(BlockTextures::kAcaciaPlank),
	        BlockTransparency::kOpaque,
	        (LightLvl)0,
	        BlockCollision::kSolid,
	    },
	    {
	        "Jungle Plank",
	        BLOCK_TEXTURE_SAME(BlockTextures::kJunglePlank),
	        BlockTransparency::kOpaque,
	        (LightLvl)0,
	        BlockCollision::kSolid,
	    },
	    {
	        "Spruce Plank",
	        BLOCK_TEXTURE_SAME(BlockTextures::kSprucePlank),
	        BlockTransparency::kOpaque,
	        (LightLvl)0,
	        BlockCollision::kSolid,
	    },
	    {
	        "Birch Plank",
	        BLOCK_TEXTURE_SAME(BlockTextures::kBirchPlank),
	        BlockTransparency::kOpaque,
	        (LightLvl)0,
	        BlockCollision::kSolid,
	    },
	};
};
