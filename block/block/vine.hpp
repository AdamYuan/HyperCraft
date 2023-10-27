#pragma once
#include "public/InnerSurfaceMesh.hpp"
#include "public/Trait.hpp"

template <> struct BlockTrait<kVine> : public SingleBlockTrait<kVine> {
	constexpr static BlockProperty kProperty = {
	    "Vine", BLOCK_TEXTURE_NONE, BlockTransparency::kTransparent, (LightLvl)0, BlockCollision::kNone,
	};

	inline constexpr static uint8_t kTransforms = 6;
	template <BlockMeta Transform> inline static constexpr BlockProperty TransformProperty(BlockProperty property) {
		property.p_custom_mesh = &kInnerSurfaceMesh<BlockTextures::kVine, Transform, 1>;
		return property;
	}
};
