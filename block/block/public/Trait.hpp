#pragma once

#include <bit>
#include <block/Block.hpp>
#include <cinttypes>
#include <texture/BlockTexture.hpp>

using namespace hc::block;
using namespace hc::texture;

#define BLOCK_TEXTURE_NONE \
	{ \
		{}, {}, {}, {}, {}, {} \
	}
#define BLOCK_TEXTURE_SAME(x) \
	{ x, x, x, x, x, x }
#define BLOCK_TEXTURE_BOT_SIDE_TOP(b, s, t) \
	{ s, s, t, b, s, s }

enum : BlockID {
#include "../../register/blocks"
};

template <BlockID> struct BlockTrait {
	inline static constexpr uint8_t kVariants = 0;
	inline static constexpr uint8_t kTransforms = 0;

	inline static constexpr uint8_t kVariantBits = 0;
	inline static constexpr uint8_t kTransformBits = 0;

	template <BlockMeta Variant, BlockMeta Transform> inline static constexpr BlockProperty GetProperty() {
		return {"None", BLOCK_TEXTURE_NONE, BlockTransparencies::kTransparent, BlockCollisionBits::kNone};
	}
};

template <BlockID ID> struct SingleBlockTrait {
	inline static constexpr uint8_t kVariants = 1;
	inline static constexpr uint8_t kVariantBits = 0;
	inline static constexpr uint8_t kTransformBits = std::countr_zero(std::bit_ceil(BlockTrait<ID>::kTransforms));

	inline static constexpr uint8_t kTransforms = 0;
	template <BlockMeta Transform> inline static constexpr BlockProperty TransformProperty(BlockProperty property) {
		return property;
	}

	template <BlockMeta Variant, BlockMeta Transform> inline static constexpr BlockProperty GetProperty() {
		return BlockTrait<ID>::template TransformProperty<Transform>(BlockTrait<ID>::kProperty);
	}
};

template <BlockID ID> struct MultiBlockTrait {
	inline static constexpr uint8_t kVariants = sizeof(BlockTrait<ID>::kProperties) / sizeof(BlockProperty);
	inline static constexpr uint8_t kVariantBits = std::countr_zero(std::bit_ceil(kVariants));
	inline static constexpr uint8_t kTransformBits = std::countr_zero(std::bit_ceil(BlockTrait<ID>::kTransforms));

	inline static constexpr uint8_t kTransforms = 0;
	template <BlockMeta Transform> inline static constexpr BlockProperty TransformProperty(BlockProperty property) {
		return property;
	}

	template <BlockMeta Variant, BlockMeta Transform> inline static constexpr BlockProperty GetProperty() {
		return BlockTrait<ID>::template TransformProperty<Transform>(
		    BlockTrait<ID>::kProperties[Variant >= kVariants ? 0 : Variant]);
	}
};