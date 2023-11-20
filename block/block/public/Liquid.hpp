#pragma once

#include "Trait.hpp"
#include <algorithm>
#include <block/Block.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <texture/BlockTexture.hpp>

namespace hc::block {
template <typename T>
concept LiquidConcept = requires(T t) {
	requires std::same_as<decltype(T::IsObstacle(Block{})), bool>;
	requires std::same_as<decltype(T::kSourceID), const BlockID>;
	requires std::same_as<decltype(T::kFlowID), const BlockID>;
	requires std::same_as<decltype(T::kSourceName), const char *const>;
	requires std::same_as<decltype(T::kFlowName), const char *const>;
	requires std::same_as<decltype(T::kFlowDistance), const uint32_t>;
	requires std::same_as<decltype(T::kDetectDistance), const uint32_t>;
	requires std::same_as<decltype(T::kFlowTickInterval), const uint32_t>;
	requires std::same_as<decltype(T::kRegenerateSource), const bool>;
	requires std::same_as<decltype(T::kSourceTex), const BlockTexID>;
	requires std::same_as<decltype(T::kFlowTex), const BlockTexID>;
	requires std::same_as<decltype(T::kTransparency), const BlockTransparency>;
	requires std::same_as<decltype(T::kLightLvl), const LightLvl>;
};

template <LiquidConcept Config>
inline static void on_liquid_update(const Block *p_neighbours, Block *p_out_set_blocks,
                                    std::function<Block(glm::i8vec3)> extra_get_block_func) {
	Block blk = p_neighbours[6];
	const auto is_source = [p_neighbours](int i = 6) { return p_neighbours[i].GetID() == Config::kSourceID; };
	const auto is_flow = [p_neighbours](int i = 6) { return p_neighbours[i].GetID() == Config::kFlowID; };
	const auto is_liquid = [p_neighbours, &is_source, &is_flow](int i = 6) { return is_source(i) || is_flow(i); };
	const auto is_wall = [p_neighbours, &is_liquid](int i = 6) {
		return Config::IsObstacle(p_neighbours[i]) && !is_liquid(i);
	};
	const auto is_wall_blk = [](Block blk) {
		return Config::IsObstacle(blk) && blk.GetID() != Config::kSourceID && blk.GetID() != Config::kFlowID;
	};
	const auto get_lvl = [p_neighbours, &is_source, &is_flow](int i = 6) -> int8_t {
		return is_source(i) ? Config::kFlowDistance : (is_flow(i) ? p_neighbours[i].GetMeta() : -1);
	};

	if (!is_liquid())
		return;

	// Flow -> Source
	if constexpr (Config::kRegenerateSource) {
		if (!is_source() && uint32_t(is_source(BlockFaces::kLeft) + is_source(BlockFaces::kRight) +
		                             is_source(BlockFaces::kBack) + is_source(BlockFaces::kFront)) >= 2) {
			p_out_set_blocks[6] = Config::kSourceID;
			return;
		}
	}

	int8_t lvl = get_lvl();

	int8_t lvl_left = get_lvl(BlockFaces::kLeft), lvl_right = get_lvl(BlockFaces::kRight),
	       lvl_back = get_lvl(BlockFaces::kBack), lvl_front = get_lvl(BlockFaces::kFront);

	// Down Shrink
	if (!is_liquid(BlockFaces::kTop) && is_flow()) {
		if (!is_liquid(BlockFaces::kLeft) && !is_liquid(BlockFaces::kRight) && !is_liquid(BlockFaces::kBack) &&
		    !is_liquid(BlockFaces::kFront)) {
			p_out_set_blocks[6] = Blocks::kAir;
			return;
		} else if (lvl == Config::kFlowDistance) {
			if (is_wall(BlockFaces::kBottom)) {
				auto max_lvl = std::max(std::max(lvl_left, lvl_right), std::max(lvl_back, lvl_front));
				Block new_blk = max_lvl == 0 ? Blocks::kAir : Block{Config::kFlowID, BlockMeta(max_lvl - 1), 0};
				p_out_set_blocks[6] = new_blk;
			} else
				p_out_set_blocks[6] = Blocks::kAir;
			return;
		}
	}

	// Shrink
	if (is_flow() && lvl < Config::kFlowDistance && lvl >= lvl_left && lvl >= lvl_right && lvl >= lvl_back &&
	    lvl >= lvl_front) {
		auto max_lvl = std::max(std::max(lvl_left, lvl_right), std::max(lvl_back, lvl_front));
		if (~max_lvl) {
			Block new_blk = max_lvl == 0 ? Blocks::kAir : Block{Config::kFlowID, BlockMeta(max_lvl - 1), 0};
			p_out_set_blocks[6] = new_blk;
		}
		return;
	}

	if (is_wall(BlockFaces::kBottom) || is_source(BlockFaces::kBottom)) {
		// Spread
		if (lvl > 0) {
			static_assert(Config::kDetectDistance <= Config::kFlowDistance);

			const auto detect_length = [&is_wall_blk, &is_wall, &is_source,
			                            &extra_get_block_func](BlockFace dir) -> uint32_t {
				int8_t axis = dir >> 1, delta = 1 - ((dir & 1) << 1);
				glm::i8vec3 cur = {}, gnd = {0, -1, 0};

				cur[axis] += delta, gnd[axis] += delta;
				if (is_wall(dir) || is_source(dir))
					return -1; // mark
				else if (!is_wall_blk(extra_get_block_func(gnd)))
					return 1;

				for (uint32_t i = 1; i < Config::kDetectDistance; ++i) {
					cur[axis] += delta, gnd[axis] += delta;
					auto cur_blk = extra_get_block_func(cur);
					if (is_wall_blk(cur_blk) || cur_blk.GetID() == Config::kSourceID)
						return Config::kDetectDistance + 1;
					else if (!is_wall_blk(extra_get_block_func(gnd)))
						return i + 1;
				}
				return Config::kDetectDistance + 1;
			};
			uint32_t len_left = detect_length(BlockFaces::kLeft), len_right = detect_length(BlockFaces::kRight),
			         len_back = detect_length(BlockFaces::kBack), len_front = detect_length(BlockFaces::kFront);

			uint32_t len_min = std::min(std::min(len_left, len_right), std::min(len_back, len_front));

			if ((~len_left) && lvl > lvl_left + 1 && len_left == len_min)
				p_out_set_blocks[BlockFaces::kLeft] = Block{Config::kFlowID, BlockMeta(lvl - 1), 0};
			if ((~len_right) && lvl > lvl_right + 1 && len_right == len_min)
				p_out_set_blocks[BlockFaces::kRight] = Block{Config::kFlowID, BlockMeta(lvl - 1), 0};
			if ((~len_back) && lvl > lvl_back + 1 && len_back == len_min)
				p_out_set_blocks[BlockFaces::kBack] = Block{Config::kFlowID, BlockMeta(lvl - 1), 0};
			if ((~len_front) && lvl > lvl_front + 1 && len_front == len_min)
				p_out_set_blocks[BlockFaces::kFront] = Block{Config::kFlowID, BlockMeta(lvl - 1), 0};
		}
	} else if (!is_source(BlockFaces::kBottom)) {
		// Down Propagation
		p_out_set_blocks[BlockFaces::kBottom] = Block{Config::kFlowID, Config::kFlowDistance, 0};
	}
}

template <LiquidConcept Config>
inline static BlockEvent kLiquidEvent = {
    Config::kFlowTickInterval,
    {
        glm::i8vec3{1, 0, 0},
        glm::i8vec3{-1, 0, 0},
        glm::i8vec3{0, 1, 0},
        glm::i8vec3{0, -1, 0},
        glm::i8vec3{0, 0, 1},
        glm::i8vec3{0, 0, -1},
        glm::i8vec3{0, 0, 0},
    },
    7u,
    on_liquid_update<Config>,
};

template <LiquidConcept Config> inline static constexpr uint8_t get_flow_height(BlockMeta variant) {
	static_assert(16 % Config::kFlowDistance == 0);
	return std::max(variant * (16 / Config::kFlowDistance), 1u);
}

template <LiquidConcept Config, BlockMeta Variant>
inline static constexpr void flow_mesh_gen(const Block *neighbours, BlockMeshFace *p_out_faces,
                                           uint32_t *p_out_face_count, texture::BlockTexture p_out_textures[6],
                                           bool *p_dynamic_texture) {
	//  0 1
	//  2 3
	uint8_t h0, h1, h2, h3;
	{
		const auto get_nei_height = [](Block block) -> int8_t {
			return block.GetID() == Config::kSourceID
			           ? 16
			           : (block.GetID() == Config::kFlowID ? get_flow_height<Config>(block.GetMeta())
			                                               : (Config::IsObstacle(block) ? -1 : 0));
		};
		int8_t nhs[] =
		    {
		        get_nei_height(neighbours[0]), get_nei_height(neighbours[1]), get_nei_height(neighbours[2]),
		        get_nei_height(neighbours[3]), get_nei_height(neighbours[4]), get_nei_height(neighbours[5]),
		        get_nei_height(neighbours[6]), get_nei_height(neighbours[7]),
		    },
		       nb = get_flow_height<Config>(Variant);

		// 0 1 2
		// 3   4
		// 5 6 7
		const auto get_vh = [&nhs, nb](uint32_t a, uint32_t b, uint32_t c) -> uint8_t {
			uint8_t mx = std::max(std::max(nb, nhs[a]), std::max(nhs[b], nhs[c]));
			if (mx != 16 && (nb == 0) + (nhs[a] == 0) + (nhs[b] == 0) + (nhs[c] == 0) >= 3)
				return 0;
			return mx;
		};
		h0 = get_vh(0, 1, 3);
		h1 = get_vh(1, 2, 4);
		h2 = get_vh(3, 5, 6);
		h3 = get_vh(4, 6, 7);
	}

	*p_dynamic_texture = true;
	if (h0 == 16 && h1 == 16 && h2 == 16 && h3 == 16) {
		*p_out_face_count = 0;
		std::fill(p_out_textures, p_out_textures + 6, Config::kFlowTex);
	} else {
		*p_out_face_count = 1;
		p_out_faces[0] = BlockMeshFace{
		    1,
		    BlockFaces::kTop,
		    BlockFaces::kTop,
		    {Config::kFlowTex},
		    {
		        {(uint8_t)0, h0, (uint8_t)0},
		        {(uint8_t)0, h2, (uint8_t)16},
		        {(uint8_t)16, h3, (uint8_t)16},
		        {(uint8_t)16, h1, (uint8_t)0},
		    },
		};
		bool left_full = h0 == 16 && h2 == 16, left_empty = h0 == 0 && h2 == 0;
		bool right_full = h1 == 16 && h3 == 16, right_empty = h1 == 0 && h3 == 0;
		bool front_full = h2 == 16 && h3 == 16, front_empty = h2 == 0 && h3 == 0;
		bool back_full = h0 == 16 && h1 == 16, back_empty = h0 == 0 && h1 == 0;
		if (!left_full && !left_empty && neighbours[3].GetID() != Config::kSourceID &&
		    neighbours[3].GetID() != Config::kFlowID) {
			p_out_faces[(*p_out_face_count)++] = BlockMeshFace{
			    0,
			    BlockFaces::kLeft,
			    BlockFaces::kLeft,
			    {Config::kFlowTex},
			    {
			        {(uint8_t)0, h0, (uint8_t)0},
			        {(uint8_t)0, (uint8_t)0, (uint8_t)0},
			        {(uint8_t)0, (uint8_t)0, (uint8_t)16},
			        {(uint8_t)0, h2, (uint8_t)16},
			    },
			};
		}
		if (!right_full && !right_empty && neighbours[4].GetID() != Config::kSourceID &&
		    neighbours[4].GetID() != Config::kFlowID) {
			p_out_faces[(*p_out_face_count)++] = BlockMeshFace{
			    0,
			    BlockFaces::kRight,
			    BlockFaces::kRight,
			    {Config::kFlowTex},
			    {
			        {(uint8_t)16, (uint8_t)0, (uint8_t)0},
			        {(uint8_t)16, h1, (uint8_t)0},
			        {(uint8_t)16, h3, (uint8_t)16},
			        {(uint8_t)16, (uint8_t)0, (uint8_t)16},
			    },
			};
		}
		if (!back_full && !back_empty && neighbours[1].GetID() != Config::kSourceID &&
		    neighbours[1].GetID() != Config::kFlowID) {
			p_out_faces[(*p_out_face_count)++] = BlockMeshFace{
			    2,
			    BlockFaces::kBack,
			    BlockFaces::kBack,
			    {Config::kFlowTex},
			    {
			        {(uint8_t)0, (uint8_t)0, (uint8_t)0},
			        {(uint8_t)0, h0, (uint8_t)0},
			        {(uint8_t)16, h1, (uint8_t)0},
			        {(uint8_t)16, (uint8_t)0, (uint8_t)0},
			    },
			};
		}
		if (!front_full && !front_empty && neighbours[6].GetID() != Config::kSourceID &&
		    neighbours[6].GetID() != Config::kFlowID) {
			p_out_faces[(*p_out_face_count)++] = BlockMeshFace{
			    2,
			    BlockFaces::kFront,
			    BlockFaces::kFront,
			    {Config::kFlowTex},
			    {
			        {(uint8_t)0, h2, (uint8_t)16},
			        {(uint8_t)0, (uint8_t)0, (uint8_t)16},
			        {(uint8_t)16, (uint8_t)0, (uint8_t)16},
			        {(uint8_t)16, h3, (uint8_t)16},
			    },
			};
		}
		p_out_textures[BlockFaces::kTop] = texture::BlockTextures::kNone;
		p_out_textures[BlockFaces::kLeft] = left_full ? Config::kFlowTex : texture::BlockTextures::kNone;
		p_out_textures[BlockFaces::kRight] = right_full ? Config::kFlowTex : texture::BlockTextures::kNone;
		p_out_textures[BlockFaces::kFront] = front_full ? Config::kFlowTex : texture::BlockTextures::kNone;
		p_out_textures[BlockFaces::kBack] = back_full ? Config::kFlowTex : texture::BlockTextures::kNone;
		p_out_textures[BlockFaces::kBottom] = Config::kFlowTex;
	}
}

template <LiquidConcept Config, BlockMeta Variant>
inline static constexpr BlockMesh kLiquidFlowMesh = {
    {},
    0,
    {{{0, 0, 0}, {16, get_flow_height<Config>(Variant), 16}}},
    1,
    flow_mesh_gen<Config, Variant>,
    {
        glm::i8vec3{-1, 0, -1},
        glm::i8vec3{0, 0, -1},
        glm::i8vec3{1, 0, -1},
        glm::i8vec3{-1, 0, 0},
        glm::i8vec3{1, 0, 0},
        glm::i8vec3{-1, 0, 1},
        glm::i8vec3{0, 0, 1},
        glm::i8vec3{1, 0, 1},
    },
    8u,
};

template <LiquidConcept Config> struct LiquidFlowTrait {
	inline static constexpr uint8_t kVariants = Config::kFlowDistance + 1;
	inline static constexpr uint8_t kVariantBits = std::countr_zero(std::bit_ceil(kVariants));
	inline static constexpr uint8_t kTransforms = 0;
	inline static constexpr uint8_t kTransformBits = std::countr_zero(std::bit_ceil(kTransforms));

	template <BlockMeta Transform> inline static constexpr BlockProperty TransformProperty(BlockProperty property) {
		return property;
	}

	template <BlockMeta Variant, BlockMeta Transform> inline static constexpr BlockProperty GetProperty() {
		return {
		    Config::kFlowName,      {},
		    Config::kTransparency,  Config::kLightLvl,
		    BlockCollision::kWater, &kLiquidFlowMesh<Config, Variant>,
		    &kLiquidEvent<Config>,
		};
	}
};

template <LiquidConcept Config> struct LiquidSourceTrait : public SingleBlockTrait<Config::kSourceID> {
	constexpr static BlockProperty kProperty = {
	    Config::kSourceName,    BLOCK_TEXTURE_SAME(Config::kSourceTex),
	    Config::kTransparency,  Config::kLightLvl,
	    BlockCollision::kWater, nullptr,
	    &kLiquidEvent<Config>,
	};
};

} // namespace hc::block
