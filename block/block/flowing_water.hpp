#pragma once
#include "public/Water.hpp"

template <> struct BlockTrait<kFlowingWater> : public LiquidFlowTrait<WaterTrait> {};
