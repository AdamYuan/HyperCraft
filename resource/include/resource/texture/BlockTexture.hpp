#ifndef CUBECRAFT3_RESOURCE_BLOCK_TEXTURE_HPP
#define CUBECRAFT3_RESOURCE_BLOCK_TEXTURE_HPP

#include <cinttypes>

using BlockTexID = uint16_t;
using BlockTexRot = uint8_t;

class BlockTexture {
private:
	uint16_t m_data;

public:
	inline constexpr BlockTexture() : m_data{} {}
	inline constexpr BlockTexture(BlockTexID id) : m_data{id} {}
	inline constexpr BlockTexture(BlockTexID id, BlockTexRot rot) : m_data((rot << 14u) | id) {}

	inline constexpr BlockTexRot GetRotation() const { return m_data >> 14u; }
	inline constexpr BlockTexID GetID() const { return m_data & 0x3fffu; }

	inline constexpr uint16_t GetData() const { return m_data; }
	inline constexpr bool Empty() const { return m_data == 0; }

	inline constexpr bool IsTransparent() const {
#include <generated/block_texture_transparency.inl>
		return kBlockTextureTransparency[GetID()];
	}

	bool operator==(BlockTexture r) const { return m_data == r.m_data; }
	bool operator!=(BlockTexture r) const { return m_data != r.m_data; }
};

#include <generated/block_texture_png.inl>

struct BlockTextures {
	enum : BlockTexID {
#include <block_texture_enum.inl>
	};
	enum ROT : BlockTexRot { kRot0 = 0, kRot90, kRot180, kRot270 };
};

#endif
