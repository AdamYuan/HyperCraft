#pragma once

#include "BlockProperty.hpp"

namespace hc::block {

struct Blocks {
	enum : BlockID {
#include "../../register/blocks"
	};
};

struct BlockVariants {
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
			uint8_t m_meta, m_id;
#else
			uint8_t m_id, m_meta;
#endif
		};
	};

	static const BlockProperty *kBlockDataProperties;
	static const uint8_t *kBlockIDVariantBits;
	static const uint8_t *kBlockIDTransformBits;

	inline static constexpr u8AABB kDefaultAABB{{0, 0, 0}, {16, 16, 16}};

	inline const BlockProperty *get_property() const { return kBlockDataProperties + m_data; }

public:
	inline Block() : m_data{} {}
	inline Block(BlockID id) : m_id{id}, m_meta{} {}
	inline Block(BlockID id, BlockMeta variant, BlockMeta transform)
	    : m_id{id}, m_meta(variant | (transform << kBlockIDVariantBits[id])) {}

	inline BlockID GetID() const { return m_id; }
	// inline void SetID(BlockID id) { m_id = id; }
	inline BlockMeta GetMeta() const { return m_meta; }
	// inline void SetMeta(BlockMeta meta) { m_meta = meta; }
	inline BlockMeta GetVariant() const { return m_meta & ((1u << kBlockIDVariantBits[m_id]) - 1); }
	inline BlockMeta GetTransform() const { return m_meta >> kBlockIDVariantBits[m_id]; }

	inline constexpr uint16_t GetData() const { return m_data; }
	// inline void SetData(uint16_t data) { m_data = data; }

	inline bool HaveCustomMesh() const { return get_property()->p_custom_mesh; }
	inline const BlockMesh *GetCustomMesh() const { return get_property()->p_custom_mesh; }
	inline const u8AABB *GetAABBs() const { return HaveCustomMesh() ? GetCustomMesh()->aabbs : &kDefaultAABB; }
	inline uint32_t GetAABBCount() const { return HaveCustomMesh() ? GetCustomMesh()->aabb_count : 1; }
	inline const char *GetName() const { return get_property()->name; }
	inline texture::BlockTexture GetTexture(BlockFace face) const { return get_property()->textures[face]; }
	// Vertical Sunlight
	inline BlockTransparency GetTransparency() const { return get_property()->transparency; }
	inline bool GetVerticalLightPass() const { return GetTransparency() == BlockTransparency::kTransparent; }
	inline bool GetIndirectLightPass() const { return GetTransparency() != BlockTransparency::kOpaque; }
	inline LightLvl GetLightLevel() const { return get_property()->light_level; }

	inline BlockCollisionMask GetCollisionMask() const { return get_property()->collision_mask; }

	inline bool ShowFace(BlockFace face, Block neighbour) const {
		auto tex = GetTexture(face), nei_tex = neighbour.GetTexture(BlockFaceOpposite(face));
		if (tex.Empty() || tex == nei_tex)
			return false;
		if (!tex.IsTransparent() && !nei_tex.IsTransparent())
			return false;
		if (tex.IsLiquid() && !nei_tex.Empty())
			return false;
		return !tex.IsTransparent() || nei_tex.IsTransparent() || nei_tex.IsLiquid();
	}

	inline bool operator==(Block r) const { return m_data == r.m_data; }
	inline bool operator!=(Block r) const { return m_data != r.m_data; }
};
static_assert(sizeof(Block) == 2);

} // namespace hc::block
