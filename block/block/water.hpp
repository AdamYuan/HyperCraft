#pragma once
#include "public/Water.hpp"

template <> struct BlockTrait<kWater> : public LiquidSourceTrait<WaterTrait> {};
