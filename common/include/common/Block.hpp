#ifndef CUBECRAFT3_COMMON_BLOCK_HPP
#define CUBECRAFT3_COMMON_BLOCK_HPP

#include <cinttypes>
#include <common/BlockMesh.hpp>
#include <common/Endian.hpp>
#include <iterator>
#include <limits>
#include <resource/texture/BlockTexture.hpp>
#include <type_traits>

using BlockID = uint8_t;
using BlockMeta = uint8_t;

using BlockTransparency = uint8_t;
struct BlockTransparencies {
	enum : uint8_t { kOpaque = 0, kSemiTransparent, kTransparent };
};
using BlockCollisionMask = uint8_t;
struct BlockCollisionBits {
	enum : uint8_t { kNone = 1 << 0, kSolid = 1 << 1, kLiquid = 1 << 2 };
};
struct BlockProperty {
	const char *name{"Unnamed"};
	BlockTexture textures[6]{};
	// bool indirect_light_pass{false}, vertical_light_pass{false};
	BlockTransparency transparency{BlockTransparencies::kOpaque};
	BlockCollisionMask collision_mask{BlockCollisionBits::kSolid};
	BlockMesh custom_mesh;
	// META path #1: array to properties
	const BlockProperty *meta_property_array{nullptr};
	BlockMeta meta_property_array_count{};
	// META path #2: function pointer
	const BlockProperty *(*meta_property_func)(BlockMeta meta){nullptr};

	inline constexpr BlockProperty RotateCW(uint8_t axis) const {
		if (axis == 0) {
			return {name,
			        {
			            textures[0].RotateCW(),
			            textures[1].RotateCCW(),
			            textures[BlockFaces::kFront],
			            textures[BlockFaces::kBack],
			            textures[BlockFaces::kBottom],
			            textures[BlockFaces::kTop],
			        },
			        transparency,
			        collision_mask,
			        custom_mesh};
		} else if (axis == 1) {
			return {name,
			        {
			            textures[BlockFaces::kBack],
			            textures[BlockFaces::kFront],
			            textures[2].RotateCW(),
			            textures[3].RotateCCW(),
			            textures[BlockFaces::kRight],
			            textures[BlockFaces::kLeft],
			        },
			        transparency,
			        collision_mask,
			        custom_mesh};
		} else {
			return {name,
			        {
			            textures[BlockFaces::kTop],
			            textures[BlockFaces::kBottom],
			            textures[BlockFaces::kLeft].TransSwapUV(),
			            textures[BlockFaces::kRight].TransSwapUV(),
			            textures[4].RotateCW(),
			            textures[5].RotateCCW(),
			        },
			        transparency,
			        collision_mask,
			        custom_mesh};
		}
	}
};

#define BLOCK_TEXTURE_SAME(x) \
	{ x, x, x, x, x, x }
#define BLOCK_TEXTURE_BOT_SIDE_TOP(b, s, t) \
	{ s, s, t, b, s, s }
#define BLOCK_PROPERTY_ARRAY_SIZE(array) (sizeof(array) / sizeof(BlockProperty))
#define BLOCK_PROPERTY_META_ARRAY(generic_name, meta_prop_array) \
	{ generic_name, {}, {}, {}, {}, meta_prop_array, sizeof(meta_prop_array) / sizeof(BlockProperty) }
#define BLOCK_PROPERTY_META_FUNCTION(generic_name, meta_prop_func) \
	{ generic_name, {}, {}, {}, {}, nullptr, 0, meta_prop_func }

struct Blocks {
	enum ID : BlockID {
		kAir = 0,
		kStone,
		kCobblestone,
		kDirt,
		kGrassBlock,
		kGrass,
		kSand,
		kDeadBush,
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
		kPlank,

		// Decorations
		kApple,
		kCactus,
		kVine,
		kRedMushroom,
		kBrownMushroom,
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
public:
	inline static constexpr Block MakeTreeLog(BlockMeta tree_meta, uint8_t axis = 1) {
		axis = (axis + 1u) % 3u;
		return {Blocks::kLog, (BlockMeta)(tree_meta | (axis << 4u))};
	}

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
	     {},
	     BlockTransparencies::kTransparent,
	     BlockCollisionBits::kNone,
	     BlockMeshes::Cross(BlockTextures::kGrassPlain, 8, 0, 16)},
	    {"Grass",
	     {},
	     BlockTransparencies::kTransparent,
	     BlockCollisionBits::kNone,
	     BlockMeshes::Cross(BlockTextures::kGrassSavanna, 8, 0, 16)},
	    {"Grass",
	     {},
	     BlockTransparencies::kTransparent,
	     BlockCollisionBits::kNone,
	     BlockMeshes::Cross(BlockTextures::kGrassTropical, 8, 0, 16)},
	    {"Grass",
	     {},
	     BlockTransparencies::kTransparent,
	     BlockCollisionBits::kNone,
	     BlockMeshes::Cross(BlockTextures::kGrassBoreal, 8, 0, 16)},
	};

	inline static constexpr BlockProperty kGrassBlockProperties[] = {
	    {"Grass Block",
	     BLOCK_TEXTURE_BOT_SIDE_TOP(BlockTextures::kDirt, BlockTextures::kGrassPlainSide,
	                                BlockTextures::kGrassPlainTop),
	     BlockTransparencies::kOpaque, BlockCollisionBits::kSolid}, //
	    {"Grass Block",
	     BLOCK_TEXTURE_BOT_SIDE_TOP(BlockTextures::kDirt, BlockTextures::kGrassSavannaSide,
	                                BlockTextures::kGrassSavannaTop),
	     BlockTransparencies::kOpaque, BlockCollisionBits::kSolid}, //
	    {"Grass Block",
	     BLOCK_TEXTURE_BOT_SIDE_TOP(BlockTextures::kDirt, BlockTextures::kGrassTropicalSide,
	                                BlockTextures::kGrassTropicalTop),
	     BlockTransparencies::kOpaque, BlockCollisionBits::kSolid}, //
	    {"Grass Block",
	     BLOCK_TEXTURE_BOT_SIDE_TOP(BlockTextures::kDirt, BlockTextures::kGrassBorealSide,
	                                BlockTextures::kGrassBorealTop),
	     BlockTransparencies::kOpaque, BlockCollisionBits::kSolid}, //
	};
	inline static constexpr BlockProperty kLeavesProperties[] = {
	    {"Oak Leaves", BLOCK_TEXTURE_SAME(BlockTextures::kOakLeaves), BlockTransparencies::kSemiTransparent,
	     BlockCollisionBits::kSolid}, //
	    {"Acacia Leaves", BLOCK_TEXTURE_SAME(BlockTextures::kAcaciaLeaves), BlockTransparencies::kSemiTransparent,
	     BlockCollisionBits::kSolid}, //
	    {"Jungle Leaves", BLOCK_TEXTURE_SAME(BlockTextures::kJungleLeaves), BlockTransparencies::kSemiTransparent,
	     BlockCollisionBits::kSolid}, //
	    {"Spruce Leaves", BLOCK_TEXTURE_SAME(BlockTextures::kSpruceLeaves), BlockTransparencies::kSemiTransparent,
	     BlockCollisionBits::kSolid}, //
	    {"Birch Leaves", BLOCK_TEXTURE_SAME(BlockTextures::kBirchLeaves), BlockTransparencies::kSemiTransparent,
	     BlockCollisionBits::kSolid}, //
	};

	inline static constexpr BlockProperty kLogProperties[] = {
	    {"Oak Log",
	     BLOCK_TEXTURE_BOT_SIDE_TOP(BlockTextures::kOakLogTop, BlockTextures::kOakLog, BlockTextures::kOakLogTop),
	     BlockTransparencies::kOpaque, BlockCollisionBits::kSolid}, //
	    {"Acacia Log",
	     BLOCK_TEXTURE_BOT_SIDE_TOP(BlockTextures::kAcaciaLogTop, BlockTextures::kAcaciaLog,
	                                BlockTextures::kAcaciaLogTop),
	     BlockTransparencies::kOpaque, BlockCollisionBits::kSolid}, //
	    {"Jungle Log",
	     BLOCK_TEXTURE_BOT_SIDE_TOP(BlockTextures::kJungleLogTop, BlockTextures::kJungleLog,
	                                BlockTextures::kJungleLogTop),
	     BlockTransparencies::kOpaque, BlockCollisionBits::kSolid}, //
	    {"Spruce Log",
	     BLOCK_TEXTURE_BOT_SIDE_TOP(BlockTextures::kSpruceLogTop, BlockTextures::kSpruceLog,
	                                BlockTextures::kSpruceLogTop),
	     BlockTransparencies::kOpaque, BlockCollisionBits::kSolid}, //
	    {"Birch Log",
	     BLOCK_TEXTURE_BOT_SIDE_TOP(BlockTextures::kBirchLogTop, BlockTextures::kBirchLog, BlockTextures::kBirchLogTop),
	     BlockTransparencies::kOpaque, BlockCollisionBits::kSolid}, //
	};
	inline static constexpr BlockProperty kLogPropertiesX[] = {
	    kLogProperties[0].RotateCW(2), kLogProperties[1].RotateCW(2), kLogProperties[2].RotateCW(2),
	    kLogProperties[3].RotateCW(2), kLogProperties[4].RotateCW(2)};
	inline static constexpr BlockProperty kLogPropertiesZ[] = {
	    kLogProperties[0].RotateCW(0), kLogProperties[1].RotateCW(0), kLogProperties[2].RotateCW(0),
	    kLogProperties[3].RotateCW(0), kLogProperties[4].RotateCW(0)};
	static_assert(sizeof(kLogProperties) == sizeof(kLogPropertiesX));
	static_assert(sizeof(kLogProperties) == sizeof(kLogPropertiesZ));

	inline static constexpr const BlockProperty *get_log_property(BlockMeta meta) {
		// TODO: deal with log rotation
		constexpr const BlockProperty *kLogAxis[3] = {kLogPropertiesZ, kLogPropertiesX, kLogProperties};
		uint8_t axis = meta >> 4u, type = meta & 0xfu;
		return kLogAxis[axis < 3 ? axis : 0] + (type < BLOCK_PROPERTY_ARRAY_SIZE(kLogProperties) ? type : 0);
	}

	inline static constexpr BlockProperty kPlankProperties[] = {
	    {"Oak Plank", BLOCK_TEXTURE_SAME(BlockTextures::kOakPlank), BlockTransparencies::kOpaque,
	     BlockCollisionBits::kSolid}, //
	    {"Acacia Plank", BLOCK_TEXTURE_SAME(BlockTextures::kAcaciaPlank), BlockTransparencies::kOpaque,
	     BlockCollisionBits::kSolid}, //
	    {"Jungle Plank", BLOCK_TEXTURE_SAME(BlockTextures::kJunglePlank), BlockTransparencies::kOpaque,
	     BlockCollisionBits::kSolid}, //
	    {"Spruce Plank", BLOCK_TEXTURE_SAME(BlockTextures::kSprucePlank), BlockTransparencies::kOpaque,
	     BlockCollisionBits::kSolid}, //
	    {"Birch Plank", BLOCK_TEXTURE_SAME(BlockTextures::kBirchPlank), BlockTransparencies::kOpaque,
	     BlockCollisionBits::kSolid}, //
	};

	template <const char *Name, BlockTexID TexID, uint8_t Dist = 1>
	inline static constexpr BlockProperty kInnerSurfaceProperties[] = {
	    {Name,
	     {},
	     BlockTransparencies::kTransparent,
	     BlockCollisionBits::kNone,
	     BlockMeshes::InnerSurface(TexID, 0, Dist)},
	    {Name,
	     {},
	     BlockTransparencies::kTransparent,
	     BlockCollisionBits::kNone,
	     BlockMeshes::InnerSurface(TexID, 1, Dist)},
	    {Name,
	     {},
	     BlockTransparencies::kTransparent,
	     BlockCollisionBits::kNone,
	     BlockMeshes::InnerSurface(TexID, 2, Dist)},
	    {Name,
	     {},
	     BlockTransparencies::kTransparent,
	     BlockCollisionBits::kNone,
	     BlockMeshes::InnerSurface(TexID, 3, Dist)},
	    {Name,
	     {},
	     BlockTransparencies::kTransparent,
	     BlockCollisionBits::kNone,
	     BlockMeshes::InnerSurface(TexID, 4, Dist)},
	    {Name,
	     {},
	     BlockTransparencies::kTransparent,
	     BlockCollisionBits::kNone,
	     BlockMeshes::InnerSurface(TexID, 5, Dist)},
	};

	inline static constexpr const char kVineName[] = "Vine";
	inline static constexpr BlockProperty kProperties[] = {
	    {"Air", BLOCK_TEXTURE_SAME(BlockTextures::kNone), BlockTransparencies::kTransparent,
	     BlockCollisionBits::kNone}, //
	    {"Stone", BLOCK_TEXTURE_SAME(BlockTextures::kStone), BlockTransparencies::kOpaque,
	     BlockCollisionBits::kSolid}, //
	    {"Cobblestone", BLOCK_TEXTURE_SAME(BlockTextures::kCobblestone), BlockTransparencies::kOpaque,
	     BlockCollisionBits::kSolid},                                                                                 //
	    {"Dirt", BLOCK_TEXTURE_SAME(BlockTextures::kDirt), BlockTransparencies::kOpaque, BlockCollisionBits::kSolid}, //
	    BLOCK_PROPERTY_META_ARRAY("Grass Block", kGrassBlockProperties),
	    BLOCK_PROPERTY_META_ARRAY("Grass", kGrassProperties),
	    {"Sand", BLOCK_TEXTURE_SAME(BlockTextures::kSand), BlockTransparencies::kOpaque, BlockCollisionBits::kSolid}, //
	    {"Dead Bush",
	     {},
	     BlockTransparencies::kTransparent,
	     BlockCollisionBits::kNone,
	     BlockMeshes::Cross(BlockTextures::kDeadBush, 8, 0, 16)},
	    {"Gravel", BLOCK_TEXTURE_SAME(BlockTextures::kGravel), BlockTransparencies::kOpaque,
	     BlockCollisionBits::kSolid}, //
	    {"Glass", BLOCK_TEXTURE_SAME(BlockTextures::kGlass), BlockTransparencies::kTransparent,
	     BlockCollisionBits::kSolid},                                                                                 //
	    {"Snow", BLOCK_TEXTURE_SAME(BlockTextures::kSnow), BlockTransparencies::kOpaque, BlockCollisionBits::kSolid}, //
	    {"Blue Ice", BLOCK_TEXTURE_SAME(BlockTextures::kBlueIce), BlockTransparencies::kOpaque,
	     BlockCollisionBits::kSolid}, //
	    {"Ice", BLOCK_TEXTURE_SAME(BlockTextures::kIce), BlockTransparencies::kSemiTransparent,
	     BlockCollisionBits::kSolid}, //
	    {"Sandstone", BLOCK_TEXTURE_SAME(BlockTextures::kSandstone), BlockTransparencies::kOpaque,
	     BlockCollisionBits::kSolid}, //
	    {"Water", BLOCK_TEXTURE_SAME(BlockTextures::kWater), BlockTransparencies::kSemiTransparent,
	     BlockCollisionBits::kLiquid}, //
	    BLOCK_PROPERTY_META_ARRAY("Leaves", kLeavesProperties),
	    BLOCK_PROPERTY_META_FUNCTION("Log", get_log_property),
	    BLOCK_PROPERTY_META_ARRAY("Plank", kPlankProperties),
	    {"Apple",
	     {},
	     BlockTransparencies::kTransparent,
	     BlockCollisionBits::kNone,
	     BlockMeshes::Cross(BlockTextures::kApple, 5, 1, 15, BlockFaces::kBottom)},
	    {"Cactus", BLOCK_TEXTURE_BOT_SIDE_TOP(BlockTextures::kCactusBottom, 0, BlockTextures::kCactusTop),
	     BlockTransparencies::kSemiTransparent, BlockCollisionBits::kSolid, BlockMeshes::CactusSides()},
	    BLOCK_PROPERTY_META_ARRAY("Vine", (kInnerSurfaceProperties<kVineName, BlockTextures::kVine>)),
	    {"Red Mushroom",
	     {},
	     BlockTransparencies::kTransparent,
	     BlockCollisionBits::kNone,
	     BlockMeshes::Cross(BlockTextures::kRedMushroom, 5, 0, 12, true)},
	    {"Brow Mushroom",
	     {},
	     BlockTransparencies::kTransparent,
	     BlockCollisionBits::kNone,
	     BlockMeshes::Cross(BlockTextures::kBrownMushroom, 6, 0, 9, true)},
	};

	inline constexpr const BlockProperty *get_generic_property() const { return kProperties + m_id; }
	inline constexpr const BlockProperty *get_property() const {
		return get_generic_property()->meta_property_func
		           ? get_generic_property()->meta_property_func(m_meta)
		           : (get_generic_property()->meta_property_array
		                  ? get_generic_property()->meta_property_array +
		                        (m_meta < get_generic_property()->meta_property_array_count ? m_meta : 0)
		                  : get_generic_property());
	}
	inline static constexpr u8AABB kDefaultAABB{{0, 0, 0}, {16, 16, 16}};

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

	inline constexpr bool HaveCustomMesh() const { return get_property()->custom_mesh.face_count; }
	inline constexpr const BlockMesh *GetCustomMesh() const { return &get_property()->custom_mesh; }
	inline constexpr const u8AABB *GetAABBs() const {
		return HaveCustomMesh() ? GetCustomMesh()->aabbs : &kDefaultAABB;
	}
	inline constexpr uint32_t GetAABBCount() const { return HaveCustomMesh() ? GetCustomMesh()->aabb_count : 1; }
	inline constexpr const char *GetGenericName() const { return get_generic_property()->name; }
	inline constexpr const char *GetName() const { return get_property()->name; }
	inline constexpr BlockTexture GetTexture(BlockFace face) const { return get_property()->textures[face]; }
	// Vertical Sunlight
	inline constexpr BlockTransparency GetTransparency() const { return get_property()->transparency; }
	inline constexpr bool GetVerticalLightPass() const {
		return GetTransparency() == BlockTransparencies::kTransparent;
	}
	inline constexpr bool GetIndirectLightPass() const { return GetTransparency() != BlockTransparencies::kOpaque; }

	inline constexpr BlockCollisionMask GetCollisionMask() const { return get_property()->collision_mask; }

	inline constexpr bool ShowFace(BlockFace face, Block neighbour) const {
		BlockTexture tex = GetTexture(face), nei_tex = neighbour.GetTexture(BlockFaceOpposite(face));
		if (tex.Empty() || tex == nei_tex)
			return false;
		if (!tex.IsTransparent() && !nei_tex.IsTransparent())
			return false;
		if (tex.IsLiquid() && !nei_tex.Empty())
			return false;
		return !tex.IsTransparent() || nei_tex.IsTransparent() || nei_tex.IsLiquid();
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
