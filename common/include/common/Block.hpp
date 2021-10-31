#ifndef BLOCK_HPP
#define BLOCK_HPP

#include <cinttypes>
#include <type_traits>

using BlockFace = uint8_t;
struct BlockFaces {
	enum FACE : BlockFace { kRight = 0, kLeft, kTop, kBottom, kFront, kBack };
};
template <typename T>
inline constexpr typename std::enable_if<std::is_integral<T>::value, void>::type BlockFaceProceed(T *xyz, BlockFace f) {
	xyz[f >> 1] += 1 - ((f & 1) << 1);
}

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

	bool operator==(BlockTexture r) const { return m_data == r.m_data; }
	bool operator!=(BlockTexture r) const { return m_data != r.m_data; }
};

struct BlockTextures {
	enum ID : BlockTexID { kNone = 0, kStone, kDirt, kGrassTop, kGrassSide, kSand, kLogTop, kLogSide, kPlank, kGlass };
	enum ROT : BlockTexRot { kRot0 = 0, kRot90, kRot180, kRot270 };
};

using BlockID = uint8_t;
using BlockMeta = uint8_t;

struct BlockProperty {
	const char *m_name;
	BlockTexture m_textures[6];
	bool m_transparent, m_light_pass;
};

#define BLOCK_TEXTURE_SAME(x) \
	{ x, x, x, x, x, x }
#define BLOCK_TEXTURE_BOT_SIDE_TOP(b, s, t) \
	{ s, s, t, b, s, s }

struct Blocks {
	enum ID : BlockID { kAir = 0, kStone, kDirt, kGrass, kSand, kLog, kPlank, kGlass };
};

class Block {
private:
	union {
		uint16_t m_data;
		struct {
#ifdef LITTLE_ENDIAN
			uint8_t m_id, m_meta;
#else
			uint8_t m_meta, m_id;
#endif
		};
	};
	inline constexpr BlockProperty GetGenericProperty() const {
		constexpr BlockProperty kProperties[] = {
		    {"Air", BLOCK_TEXTURE_SAME(BlockTextures::kNone), true, true},      //
		    {"Stone", BLOCK_TEXTURE_SAME(BlockTextures::kStone), false, false}, //
		    {"Dirt", BLOCK_TEXTURE_SAME(BlockTextures::kDirt), false, false},   //
		    {"Grass",
		     BLOCK_TEXTURE_BOT_SIDE_TOP(BlockTextures::kDirt, BlockTextures::kGrassSide, BlockTextures::kGrassTop),
		     false, false},                                                   //
		    {"Sand", BLOCK_TEXTURE_SAME(BlockTextures::kSand), false, false}, //
		    {"Log", BLOCK_TEXTURE_BOT_SIDE_TOP(BlockTextures::kLogTop, BlockTextures::kLogSide, BlockTextures::kLogTop),
		     false, false},                                                     //
		    {"Plank", BLOCK_TEXTURE_SAME(BlockTextures::kPlank), false, false}, //
		    {"Glass", BLOCK_TEXTURE_SAME(BlockTextures::kGlass), true, true},   //
		};
		return kProperties[m_id];
	}

public:
	inline constexpr Block() : m_data{} {}
	inline constexpr Block(uint16_t data) : m_data{data} {}
	inline constexpr Block(BlockID id, BlockMeta meta) : m_id{id}, m_meta{meta} {}

	inline constexpr BlockID GetID() const { return m_id; }
	inline void SetID(BlockID id) { m_id = id; }
	inline constexpr BlockMeta GetMeta() const { return m_meta; }
	inline void SetMeta(BlockMeta meta) { m_meta = meta; }

	inline constexpr uint16_t GetData() const { return m_data; }
	inline void SetData(uint16_t data) { m_data = data; }

	inline constexpr const char *GetName() const { return GetGenericProperty().m_name; }
	inline constexpr BlockTexture GetTexture(BlockFace face) const { return GetGenericProperty().m_textures[face]; }
	inline constexpr bool GetTransparent() const { return GetGenericProperty().m_transparent; }
	inline constexpr bool GetLightPass() const { return GetGenericProperty().m_light_pass; }

	inline constexpr bool ShowFace(Block neighbour) const {
		if (GetID() == Blocks::kAir)
			return false;
		if (!GetTransparent() && !neighbour.GetTransparent())
			return false;
		if (GetTransparent() && neighbour.GetTransparent() && GetID() != neighbour.GetID() &&
		    neighbour.GetID() == Blocks::kGlass)
			return true;
		return !(GetTransparent() && neighbour.GetID() != Blocks::kAir);
	}

	bool operator==(Block r) const { return m_data == r.m_data; }
	bool operator!=(Block r) const { return m_data != r.m_data; }
};

#undef BLOCK_TEXTURE_SAME
#undef BLOCK_TEXTURE_BOT_SIDE_TOP

#endif
