#pragma once
#include "public/Trait.hpp"
#include "public/LiquidEvent.hpp"

#include <algorithm>

inline static constexpr uint8_t get_flowing_water_height(BlockMeta variant) { return std::max(variant * 2, 1); }

template <BlockMeta Variant>
inline static constexpr void flowing_water_mesh_gen(const Block *neighbours, BlockMeshFace *p_out_faces,
                                                    uint32_t *p_out_face_count, BlockTexture p_out_textures[6],
                                                    bool *p_dynamic_texture) {
	const auto get_nei_height = [](Block block) -> uint8_t {
		return block.GetID() == kWater
		           ? 16
		           : (block.GetID() == kFlowingWater ? get_flowing_water_height(block.GetMeta()) : 0);
	};
	uint8_t nhs[] =
	    {
	        get_nei_height(neighbours[0]), get_nei_height(neighbours[1]), get_nei_height(neighbours[2]),
	        get_nei_height(neighbours[3]), get_nei_height(neighbours[4]), get_nei_height(neighbours[5]),
	        get_nei_height(neighbours[6]), get_nei_height(neighbours[7]),
	    },
	        nb = get_flowing_water_height(Variant);

	// 0 1 2
	// 3   4
	// 5 6 7
	//
	//  0 1
	//  2 3
	// #define N4(a, b, c) ((nb + nhs[a] + nhs[b] + nhs[c]) / (bool(nhs[a]) + bool(nhs[b]) + bool(nhs[c]) + 1))
#define N4(a, b, c) std::max(std::max(nb, nhs[a]), std::max(nhs[b], nhs[c]))
	uint8_t h0 = N4(0, 1, 3);
	uint8_t h1 = N4(1, 2, 4);
	uint8_t h2 = N4(3, 5, 6);
	uint8_t h3 = N4(4, 6, 7);
#undef N4

	*p_dynamic_texture = true;
	if (h0 == 16 && h1 == 16 && h2 == 16 && h3 == 16) {
		*p_out_face_count = 0;
		std::fill(p_out_textures, p_out_textures + 6, BlockTextures::kWater);
	} else {
		*p_out_face_count = 1;
		p_out_faces[0] = BlockMeshFace{
		    1,
		    BlockFaces::kTop,
		    BlockFaces::kTop,
		    {BlockTextures::kWater},
		    {
		        {(uint8_t)0, h0, (uint8_t)0},
		        {(uint8_t)0, h2, (uint8_t)16},
		        {(uint8_t)16, h3, (uint8_t)16},
		        {(uint8_t)16, h1, (uint8_t)0},
		    },
		};
		bool left_full = h0 == 16 && h2 == 16;
		bool right_full = h1 == 16 && h3 == 16;
		bool front_full = h2 == 16 && h3 == 16;
		bool back_full = h0 == 16 && h1 == 16;
		if (!left_full && neighbours[3].GetID() != kWater && neighbours[3].GetID() != kFlowingWater) {
			p_out_faces[(*p_out_face_count)++] = BlockMeshFace{
			    0,
			    BlockFaces::kLeft,
			    BlockFaces::kLeft,
			    {BlockTextures::kWater},
			    {
			        {(uint8_t)0, h0, (uint8_t)0},
			        {(uint8_t)0, (uint8_t)0, (uint8_t)0},
			        {(uint8_t)0, (uint8_t)0, (uint8_t)16},
			        {(uint8_t)0, h2, (uint8_t)16},
			    },
			};
		}
		if (!right_full && neighbours[4].GetID() != kWater && neighbours[4].GetID() != kFlowingWater) {
			p_out_faces[(*p_out_face_count)++] = BlockMeshFace{
			    0,
			    BlockFaces::kRight,
			    BlockFaces::kRight,
			    {BlockTextures::kWater},
			    {
			        {(uint8_t)16, (uint8_t)0, (uint8_t)0},
			        {(uint8_t)16, h1, (uint8_t)0},
			        {(uint8_t)16, h3, (uint8_t)16},
			        {(uint8_t)16, (uint8_t)0, (uint8_t)16},
			    },
			};
		}
		if (!back_full && neighbours[1].GetID() != kWater && neighbours[1].GetID() != kFlowingWater) {
			p_out_faces[(*p_out_face_count)++] = BlockMeshFace{
			    2,
			    BlockFaces::kBack,
			    BlockFaces::kBack,
			    {BlockTextures::kWater},
			    {
			        {(uint8_t)0, (uint8_t)0, (uint8_t)0},
			        {(uint8_t)0, h0, (uint8_t)0},
			        {(uint8_t)16, h1, (uint8_t)0},
			        {(uint8_t)16, (uint8_t)0, (uint8_t)0},
			    },
			};
		}
		if (!front_full && neighbours[6].GetID() != kWater && neighbours[6].GetID() != kFlowingWater) {
			p_out_faces[(*p_out_face_count)++] = BlockMeshFace{
			    2,
			    BlockFaces::kFront,
			    BlockFaces::kFront,
			    {BlockTextures::kWater},
			    {
			        {(uint8_t)0, h2, (uint8_t)16},
			        {(uint8_t)0, (uint8_t)0, (uint8_t)16},
			        {(uint8_t)16, (uint8_t)0, (uint8_t)16},
			        {(uint8_t)16, h3, (uint8_t)16},
			    },
			};
		}
		p_out_textures[BlockFaces::kTop] = BlockTextures::kNone;
		p_out_textures[BlockFaces::kLeft] = left_full ? BlockTextures::kWater : BlockTextures::kNone;
		p_out_textures[BlockFaces::kRight] = right_full ? BlockTextures::kWater : BlockTextures::kNone;
		p_out_textures[BlockFaces::kFront] = front_full ? BlockTextures::kWater : BlockTextures::kNone;
		p_out_textures[BlockFaces::kBack] = back_full ? BlockTextures::kWater : BlockTextures::kNone;
		p_out_textures[BlockFaces::kBottom] = BlockTextures::kWater;
	}
}

template <BlockMeta Variant>
inline static constexpr BlockMesh kFlowingWaterMesh = {
    {},
    0,
    {{{0, 0, 0}, {16, get_flowing_water_height(Variant), 16}}},
    1,
    flowing_water_mesh_gen<Variant>,
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

template <> struct BlockTrait<kFlowingWater> {
	inline static constexpr uint8_t kVariants = 8;
	inline static constexpr uint8_t kVariantBits = std::countr_zero(std::bit_ceil(kVariants));
	inline static constexpr uint8_t kTransforms = 0;
	inline static constexpr uint8_t kTransformBits = std::countr_zero(std::bit_ceil(kTransforms));

	template <BlockMeta Transform> inline static constexpr BlockProperty TransformProperty(BlockProperty property) {
		return property;
	}

	template <BlockMeta Variant, BlockMeta Transform> inline static constexpr BlockProperty GetProperty() {
		return {
		    "Flowing Water",
		    BLOCK_TEXTURE_BOT_SIDE_TOP(BlockTextures::kWater, BlockTextures::kNone, BlockTextures::kNone),
		    BlockTransparency::kSemiTransparent,
		    (LightLvl)0,
		    BlockCollision::kWater,
		    &kFlowingWaterMesh<Variant>,
		};
	}
};
