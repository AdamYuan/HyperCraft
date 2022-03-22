#ifndef CUBECRAFT3_COMMON_BLOCK_HPP
#define CUBECRAFT3_COMMON_BLOCK_HPP

#include <cinttypes>
#include <common/Endian.hpp>
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
	enum ID : BlockTexID {
		kNone = 0,
		kStone,
		kCobblestone,
		kDirt,
		kGrassPlainTop,
		kGrassPlainSide,
		kGrassSavannaTop,
		kGrassSavannaSide,
		kGrassTropicalTop,
		kGrassTropicalSide,
		kGrassBorealTop,
		kGrassBorealSide,
		kSand,
		kGravel,
		kGlass,
		kSnow,
		kBlueIce,
		kIce,
		kSandstone,
		kWater,

		// TREE
		kAcaciaLeaves,
		kAcaciaLog,
		kAcaciaLogTop,
		kAcaciaPlank,

		kJungleLeaves,
		kJungleLog,
		kJungleLogTop,
		kJunglePlank,

		kOakLeaves,
		kOakLog,
		kOakLogTop,
		kOakPlank,

		kSpruceLeaves,
		kSpruceLog,
		kSpruceLogTop,
		kSprucePlank,

		kBirchLeaves,
		kBirchLog,
		kBirchLogTop,
		kBirchPlank,
	};
	enum ROT : BlockTexRot { kRot0 = 0, kRot90, kRot180, kRot270 };
};

using BlockID = uint8_t;
using BlockMeta = uint8_t;

struct BlockProperty {
	const char *m_name{"Unnamed"};
	BlockTexture m_textures[6]{};
	bool m_transparent{false}, m_light_pass{false};
	// META path #1: array to properties
	const BlockProperty *m_meta_property_array{nullptr};
	BlockMeta m_meta_property_array_count{};
	// META path #2: function pointer
	const BlockProperty *(*m_meta_property_func)(BlockMeta meta){nullptr};
};

#define BLOCK_TEXTURE_SAME(x) \
	{ x, x, x, x, x, x }
#define BLOCK_TEXTURE_BOT_SIDE_TOP(b, s, t) \
	{ s, s, t, b, s, s }
#define BLOCK_PROPERTY_ARRAY_SIZE(array) (sizeof(array) / sizeof(BlockProperty))
#define BLOCK_PROPERTY_META_ARRAY(generic_name, meta_prop_array) \
	{ generic_name, {}, {}, {}, meta_prop_array, sizeof(meta_prop_array) / sizeof(BlockProperty) }
#define BLOCK_PROPERTY_META_FUNCTION(generic_name, meta_prop_func) \
	{ generic_name, {}, {}, {}, nullptr, 0, meta_prop_func }

struct Blocks {
	enum ID : BlockID {
		kAir = 0,
		kStone,
		kCobblestone,
		kDirt,
		kGrass,
		kSand,
		kGravel,
		kGlass,
		kSnow,
		kBlueIce,
		kIce,
		kSandstone,
		kWater,

		// TREE
		kLeaves,
		kLog,
		kPlank
	};
};

struct BlockMetas {
	struct Tree {
		enum : BlockMeta { kOak = 0, kAcacia, kJungle, kSpruce, kBirch };
	};
	struct Grass {
		enum : BlockMeta { kPlain = 0, kSavanna, kTropical, kBoreal };
	};
};

class Block {
private:
	union {
		uint16_t m_data;
		struct {
#ifdef IS_SMALL_ENDIAN
			uint8_t m_id, m_meta; // regular order for little endian
#else
			uint8_t m_meta, m_id;
#endif
		};
	};

	inline static constexpr BlockProperty kGrassProperties[] = {
	    {"Grass",
	     BLOCK_TEXTURE_BOT_SIDE_TOP(BlockTextures::kDirt, BlockTextures::kGrassPlainSide,
	                                BlockTextures::kGrassPlainTop),
	     false, false}, //
	    {"Grass",
	     BLOCK_TEXTURE_BOT_SIDE_TOP(BlockTextures::kDirt, BlockTextures::kGrassSavannaSide,
	                                BlockTextures::kGrassSavannaTop),
	     false, false}, //
	    {"Grass",
	     BLOCK_TEXTURE_BOT_SIDE_TOP(BlockTextures::kDirt, BlockTextures::kGrassTropicalSide,
	                                BlockTextures::kGrassTropicalTop),
	     false, false}, //
	    {"Grass",
	     BLOCK_TEXTURE_BOT_SIDE_TOP(BlockTextures::kDirt, BlockTextures::kGrassBorealSide,
	                                BlockTextures::kGrassBorealTop),
	     false, false}, //
	};
	inline static constexpr BlockProperty kLeavesProperties[] = {
	    {"Oak Leaves", BLOCK_TEXTURE_SAME(BlockTextures::kOakLeaves), true, false},       //
	    {"Acacia Leaves", BLOCK_TEXTURE_SAME(BlockTextures::kAcaciaLeaves), true, false}, //
	    {"Jungle Leaves", BLOCK_TEXTURE_SAME(BlockTextures::kJungleLeaves), true, false}, //
	    {"Spruce Leaves", BLOCK_TEXTURE_SAME(BlockTextures::kSpruceLeaves), true, false}, //
	    {"Birch Leaves", BLOCK_TEXTURE_SAME(BlockTextures::kBirchLeaves), true, false},   //
	};

	inline static constexpr BlockProperty kLogProperties[] = {
	    {"Oak Log",
	     BLOCK_TEXTURE_BOT_SIDE_TOP(BlockTextures::kOakLogTop, BlockTextures::kOakLog, BlockTextures::kOakLogTop),
	     false, false}, //
	    {"Acacia Log",
	     BLOCK_TEXTURE_BOT_SIDE_TOP(BlockTextures::kAcaciaLogTop, BlockTextures::kAcaciaLog,
	                                BlockTextures::kAcaciaLogTop),
	     false, false}, //
	    {"Jungle Log",
	     BLOCK_TEXTURE_BOT_SIDE_TOP(BlockTextures::kJungleLogTop, BlockTextures::kJungleLog,
	                                BlockTextures::kJungleLogTop),
	     false, false}, //
	    {"Spruce Log",
	     BLOCK_TEXTURE_BOT_SIDE_TOP(BlockTextures::kSpruceLogTop, BlockTextures::kSpruceLog,
	                                BlockTextures::kSpruceLogTop),
	     false, false}, //
	    {"Birch Log",
	     BLOCK_TEXTURE_BOT_SIDE_TOP(BlockTextures::kBirchLogTop, BlockTextures::kBirchLog, BlockTextures::kBirchLogTop),
	     false, false}, //
	};
	inline static constexpr const BlockProperty *get_log_property(BlockMeta meta) {
		// TODO: deal with log rotation
		return kLogProperties + (meta < BLOCK_PROPERTY_ARRAY_SIZE(kLogProperties) ? meta : 0);
	}

	inline static constexpr BlockProperty kPlankProperties[] = {
	    {"Oak Plank", BLOCK_TEXTURE_SAME(BlockTextures::kOakPlank), false, false},       //
	    {"Acacia Plank", BLOCK_TEXTURE_SAME(BlockTextures::kAcaciaPlank), false, false}, //
	    {"Jungle Plank", BLOCK_TEXTURE_SAME(BlockTextures::kJunglePlank), false, false}, //
	    {"Spruce Plank", BLOCK_TEXTURE_SAME(BlockTextures::kSprucePlank), false, false}, //
	    {"Birch Plank", BLOCK_TEXTURE_SAME(BlockTextures::kBirchPlank), false, false},   //
	};

	inline static constexpr BlockProperty kProperties[] = {
	    {"Air", BLOCK_TEXTURE_SAME(BlockTextures::kNone), true, true},                  //
	    {"Stone", BLOCK_TEXTURE_SAME(BlockTextures::kStone), false, false},             //
	    {"Cobblestone", BLOCK_TEXTURE_SAME(BlockTextures::kCobblestone), false, false}, //
	    {"Dirt", BLOCK_TEXTURE_SAME(BlockTextures::kDirt), false, false},               //
	    BLOCK_PROPERTY_META_ARRAY("Grass", kGrassProperties),
	    {"Sand", BLOCK_TEXTURE_SAME(BlockTextures::kSand), false, false},           //
	    {"Gravel", BLOCK_TEXTURE_SAME(BlockTextures::kGravel), false, false},       //
	    {"Glass", BLOCK_TEXTURE_SAME(BlockTextures::kGlass), true, true},           //
	    {"Snow", BLOCK_TEXTURE_SAME(BlockTextures::kSnow), false, false},           //
	    {"Blue Ice", BLOCK_TEXTURE_SAME(BlockTextures::kBlueIce), false, false},    //
	    {"Ice", BLOCK_TEXTURE_SAME(BlockTextures::kIce), true, false},              //
	    {"Sandstone", BLOCK_TEXTURE_SAME(BlockTextures::kSandstone), false, false}, //
	    {"Water", BLOCK_TEXTURE_SAME(BlockTextures::kWater), true, true},           //
	    BLOCK_PROPERTY_META_ARRAY("Leaves", kLeavesProperties),
	    BLOCK_PROPERTY_META_FUNCTION("Log", get_log_property),
	    BLOCK_PROPERTY_META_ARRAY("Plank", kPlankProperties),
	};

	inline constexpr const BlockProperty *get_generic_property() const { return kProperties + m_id; }
	inline constexpr const BlockProperty *get_property() const {
		return get_generic_property()->m_meta_property_func
		           ? get_generic_property()->m_meta_property_func(m_meta)
		           : (get_generic_property()->m_meta_property_array
		                  ? get_generic_property()->m_meta_property_array +
		                        (m_meta < get_generic_property()->m_meta_property_array_count ? m_meta : 0)
		                  : get_generic_property());
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

	inline constexpr const char *GetGenericName() const { return get_generic_property()->m_name; }
	inline constexpr const char *GetName() const { return get_property()->m_name; }
	inline constexpr BlockTexture GetTexture(BlockFace face) const { return get_property()->m_textures[face]; }
	inline constexpr bool GetTransparent() const { return get_property()->m_transparent; }
	// inline constexpr bool GetLightPass() const { return get_property()->m_light_pass; }
	// Direct Light: vertical sunlight
	inline constexpr bool GetDirectLightPass() const {
		return get_property()->m_light_pass && get_property()->m_transparent;
	}
	inline constexpr bool GetIndirectLightPass() const {
		return get_property()->m_transparent || get_property()->m_light_pass;
	}

	inline constexpr bool ShowFace(Block neighbour) const {
		if (GetID() == Blocks::kAir || GetID() == neighbour.GetID())
			return false;
		if (!GetTransparent() && !neighbour.GetTransparent())
			return false;
		return !GetTransparent() || neighbour.GetID() == Blocks::kAir || neighbour.GetID() == Blocks::kWater;
	}

	bool operator==(Block r) const { return m_data == r.m_data; }
	bool operator!=(Block r) const { return m_data != r.m_data; }
};

#undef BLOCK_TEXTURE_SAME
#undef BLOCK_TEXTURE_BOT_SIDE_TOP
#undef BLOCK_PROPERTY_ARRAY_SIZE
#undef BLOCK_PROPERTY_META_ARRAY
#undef BLOCK_PROPERTY_META_FUNCTION

#endif
