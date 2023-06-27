#pragma once
#include "public/Trait.hpp"

inline static constexpr BlockMesh kCactusSides = {{
                                                      {0,
                                                       BlockFaces::kRight,
                                                       BlockFaces::kRight,
                                                       {BlockTextures::kCactusSide},
                                                       {
                                                           {15, 0, 16},
                                                           {15, 0, 0},
                                                           {15, 16, 0},
                                                           {15, 16, 16},
                                                       }},
                                                      {0,
                                                       BlockFaces::kLeft,
                                                       BlockFaces::kLeft,
                                                       {BlockTextures::kCactusSide},
                                                       {
                                                           {1, 0, 0},
                                                           {1, 0, 16},
                                                           {1, 16, 16},
                                                           {1, 16, 0},
                                                       }},
                                                      {2,
                                                       BlockFaces::kFront,
                                                       BlockFaces::kFront,
                                                       {BlockTextures::kCactusSide},
                                                       {
                                                           {0, 0, 15},
                                                           {16, 0, 15},
                                                           {16, 16, 15},
                                                           {0, 16, 15},
                                                       }},
                                                      {2,
                                                       BlockFaces::kBack,
                                                       BlockFaces::kBack,
                                                       {BlockTextures::kCactusSide},
                                                       {
                                                           {16, 0, 1},
                                                           {0, 0, 1},
                                                           {0, 16, 1},
                                                           {16, 16, 1},
                                                       }},
                                                  },
                                                  4,
                                                  {{{1, 0, 1}, {15, 16, 15}}},
                                                  1};

template <> struct BlockTrait<kCactus> : public SingleBlockTrait<kCactus> {
	constexpr static BlockProperty kProperty = {
	    "Cactus",
	    BLOCK_TEXTURE_BOT_SIDE_TOP(BlockTextures::kCactusBottom, 0, BlockTextures::kCactusTop),
	    BlockTransparencies::kSemiTransparent,
	    BlockCollisionBits::kSolid,
	    &kCactusSides,
	};
};
