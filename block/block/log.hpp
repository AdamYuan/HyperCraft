#pragma once
#include "public/Trait.hpp"

template <> struct BlockTrait<kLog> : public MultiBlockTrait<kLog> {
	inline static constexpr BlockProperty kProperties[] = {
	    {"Oak Log",
	     BLOCK_TEXTURE_BOT_SIDE_TOP(BlockTextures::kOakLogTop, BlockTextures::kOakLog, BlockTextures::kOakLogTop),
	     BlockTransparency::kOpaque, (LightLvl)0, BlockCollisionBits::kSolid},
	    {"Acacia Log",
	     BLOCK_TEXTURE_BOT_SIDE_TOP(BlockTextures::kAcaciaLogTop, BlockTextures::kAcaciaLog,
	                                BlockTextures::kAcaciaLogTop),
	     BlockTransparency::kOpaque, (LightLvl)0, BlockCollisionBits::kSolid},
	    {"Jungle Log",
	     BLOCK_TEXTURE_BOT_SIDE_TOP(BlockTextures::kJungleLogTop, BlockTextures::kJungleLog,
	                                BlockTextures::kJungleLogTop),
	     BlockTransparency::kOpaque, (LightLvl)0, BlockCollisionBits::kSolid},
	    {"Spruce Log",
	     BLOCK_TEXTURE_BOT_SIDE_TOP(BlockTextures::kSpruceLogTop, BlockTextures::kSpruceLog,
	                                BlockTextures::kSpruceLogTop),
	     BlockTransparency::kOpaque, (LightLvl)0, BlockCollisionBits::kSolid},
	    {"Birch Log",
	     BLOCK_TEXTURE_BOT_SIDE_TOP(BlockTextures::kBirchLogTop, BlockTextures::kBirchLog, BlockTextures::kBirchLogTop),
	     BlockTransparency::kOpaque, (LightLvl)0, BlockCollisionBits::kSolid},
	};

	inline static constexpr uint8_t kTransforms = 3;
	template <BlockMeta Transform> inline static constexpr BlockProperty TransformProperty(BlockProperty property) {
		if constexpr (Transform == 0)
			return property.RotateCW(2);
		if constexpr (Transform == 2)
			return property.RotateCW(0);
		else
			return property;
	}
};
