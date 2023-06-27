#pragma once
#include "public/Trait.hpp"

template <> struct BlockTrait<kGrassBlock> : public MultiBlockTrait<kGrassBlock> {
	inline static constexpr BlockProperty kProperties[] = {
	    {
	        "Grass Block",
	        BLOCK_TEXTURE_BOT_SIDE_TOP(BlockTextures::kDirt, BlockTextures::kGrassPlainSide,
	                                   BlockTextures::kGrassPlainTop),
	        BlockTransparencies::kOpaque,
	        BlockCollisionBits::kSolid,
	    },
	    {
	        "Grass Block",
	        BLOCK_TEXTURE_BOT_SIDE_TOP(BlockTextures::kDirt, BlockTextures::kGrassSavannaSide,
	                                   BlockTextures::kGrassSavannaTop),
	        BlockTransparencies::kOpaque,
	        BlockCollisionBits::kSolid,
	    },
	    {
	        "Grass Block",
	        BLOCK_TEXTURE_BOT_SIDE_TOP(BlockTextures::kDirt, BlockTextures::kGrassTropicalSide,
	                                   BlockTextures::kGrassTropicalTop),
	        BlockTransparencies::kOpaque,
	        BlockCollisionBits::kSolid,
	    },
	    {
	        "Grass Block",
	        BLOCK_TEXTURE_BOT_SIDE_TOP(BlockTextures::kDirt, BlockTextures::kGrassBorealSide,
	                                   BlockTextures::kGrassBorealTop),
	        BlockTransparencies::kOpaque,
	        BlockCollisionBits::kSolid,
	    },
	};
};
