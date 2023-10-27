#pragma once
#include "public/Trait.hpp"

template <> struct BlockTrait<kGrassBlock> : public MultiBlockTrait<kGrassBlock> {
	inline static constexpr BlockProperty kProperties[] = {
	    {
	        "Grass Block",
	        BLOCK_TEXTURE_BOT_SIDE_TOP(BlockTextures::kDirt, BlockTextures::kGrassPlainSide,
	                                   BlockTextures::kGrassPlainTop),
	        BlockTransparency::kOpaque,
	        (LightLvl)0,
	        BlockCollision::kSolid,
	    },
	    {
	        "Grass Block",
	        BLOCK_TEXTURE_BOT_SIDE_TOP(BlockTextures::kDirt, BlockTextures::kGrassSavannaSide,
	                                   BlockTextures::kGrassSavannaTop),
	        BlockTransparency::kOpaque,
	        (LightLvl)0,
	        BlockCollision::kSolid,
	    },
	    {
	        "Grass Block",
	        BLOCK_TEXTURE_BOT_SIDE_TOP(BlockTextures::kDirt, BlockTextures::kGrassTropicalSide,
	                                   BlockTextures::kGrassTropicalTop),
	        BlockTransparency::kOpaque,
	        (LightLvl)0,
	        BlockCollision::kSolid,
	    },
	    {
	        "Grass Block",
	        BLOCK_TEXTURE_BOT_SIDE_TOP(BlockTextures::kDirt, BlockTextures::kGrassBorealSide,
	                                   BlockTextures::kGrassBorealTop),
	        BlockTransparency::kOpaque,
	        (LightLvl)0,
	        BlockCollision::kSolid,
	    },
	};
};
