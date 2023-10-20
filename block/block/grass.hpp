#pragma once
#include "public/CrossMesh.hpp"
#include "public/Trait.hpp"

template <> struct BlockTrait<kGrass> : public MultiBlockTrait<kGrass> {
	inline constexpr static BlockProperty kProperties[] = {
	    {
	        "Grass",
	        BLOCK_TEXTURE_NONE,
	        BlockTransparency::kTransparent,
	        (LightLvl)0,
	        BlockCollisionBits::kNone,
	        &kCrossMesh<BlockTextures::kGrassPlain, 8, 0, 16>,
	    },
	    {
	        "Grass",
	        BLOCK_TEXTURE_NONE,
	        BlockTransparency::kTransparent,
	        (LightLvl)0,
	        BlockCollisionBits::kNone,
	        &kCrossMesh<BlockTextures::kGrassSavanna, 8, 0, 16>,
	    },
	    {
	        "Grass",
	        BLOCK_TEXTURE_NONE,
	        BlockTransparency::kTransparent,
	        (LightLvl)0,
	        BlockCollisionBits::kNone,
	        &kCrossMesh<BlockTextures::kGrassTropical, 8, 0, 16>,
	    },
	    {
	        "Grass",
	        BLOCK_TEXTURE_NONE,
	        BlockTransparency::kTransparent,
	        (LightLvl)0,
	        BlockCollisionBits::kNone,
	        &kCrossMesh<BlockTextures::kGrassBoreal, 8, 0, 16>,
	    },
	};
};
