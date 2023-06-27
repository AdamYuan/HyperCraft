#ifndef HYPERCRAFT_RESOURCE_BLOCK_TEXTURE_HPP
#define HYPERCRAFT_RESOURCE_BLOCK_TEXTURE_HPP

#include <cinttypes>

namespace hc::texture {

using BlockTexID = uint16_t;
using BlockTexTrans = uint8_t;

namespace BlockTextures {
enum : BlockTexID {
#include "../../register/block_textures"
};
enum : BlockTexTrans { kTransSwapUV = 1u, kTransNegU = 2u, kTransNegV = 4u };
} // namespace BlockTextures

class BlockTexture {
private:
	uint16_t m_data;

public:
	inline constexpr BlockTexture() : m_data{} {}
	inline constexpr BlockTexture(BlockTexID id) : m_data{id} {}
	inline constexpr BlockTexture(BlockTexID id, BlockTexTrans trans) : m_data((trans << 13u) | id) {}

	inline constexpr BlockTexTrans GetTransformation() const { return m_data >> 13u; }
	inline constexpr BlockTexID GetID() const { return m_data & 0x1fffu; }

	inline constexpr uint16_t GetData() const { return m_data; }
	inline constexpr bool Empty() const { return GetID() == 0; }

	inline constexpr bool IsTransparent() const {
#include <generated/block_texture_transparency.inl>
		return kBlockTextureTransparency[GetID()];
	}
	inline constexpr bool IsLiquid() const {
#include <generated/block_texture_transparency.inl>
		return kBlockTextureLiquid[GetID()];
	}
	inline constexpr bool UseTransparentPass() const {
#include <generated/block_texture_transparency.inl>
		return kBlockTextureTransPass[GetID()];
	}

	inline constexpr BlockTexture RotateCW() const {
		return {GetID(),
		        (BlockTexTrans)(GetTransformation() ^ (BlockTextures::kTransSwapUV | BlockTextures::kTransNegU))};
	}

	inline constexpr BlockTexture RotateCCW() const {
		return {GetID(),
		        (BlockTexTrans)(GetTransformation() ^ (BlockTextures::kTransSwapUV | BlockTextures::kTransNegV))};
	}

	inline constexpr BlockTexture TransSwapUV() const {
		return {GetID(), (BlockTexTrans)(GetTransformation() ^ BlockTextures::kTransSwapUV)};
	}

	inline constexpr BlockTexture TransNegU() const {
		return {GetID(), (BlockTexTrans)(GetTransformation() ^ BlockTextures::kTransNegU)};
	}

	inline constexpr BlockTexture TransNegV() const {
		return {GetID(), (BlockTexTrans)(GetTransformation() ^ BlockTextures::kTransNegV)};
	}

	bool operator==(BlockTexture r) const { return m_data == r.m_data; }
	bool operator!=(BlockTexture r) const { return m_data != r.m_data; }
};

#include <generated/block_texture_png.inl>

}

#endif
