#pragma once

#include "Liquid.hpp"

struct WaterTrait {
	inline static constexpr BlockID kSourceID = kWater, kFlowID = kFlowingWater;
	inline static constexpr const char *kSourceName = "Water", *kFlowName = "Flowing Water";
	inline static constexpr uint32_t kFlowDistance = 8, kDetectDistance = 6, kFlowTickInterval = 5;
	inline static constexpr bool kRegenerateSource = true;
	inline static constexpr BlockTexID kSourceTex = BlockTextures::kWater, kFlowTex = BlockTextures::kWater;
	inline static constexpr BlockTransparency kTransparency = BlockTransparency::kSemiTransparent;
	inline static constexpr LightLvl kLightLvl = 0;
	inline static bool IsObstacle(Block blk) { return blk.GetID() != kAir; }
};
static_assert(LiquidConcept<WaterTrait>);