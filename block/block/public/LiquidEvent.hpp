#pragma once

#include <block/Block.hpp>
#include <cstdio>

namespace hc::block {

template <BlockID SourceID, BlockID FlowID, uint32_t FlowDistance>
inline static void on_liquid_update(const Block *p_neighbours, BlockUpdateSetBlock *p_out_set_blocks,
                                    uint32_t *p_out_set_block_count) {
	// TODO: Improve this
	Block blk = p_neighbours[6];
	const auto is_source = [p_neighbours](int i = 6) { return p_neighbours[i].GetID() == SourceID; };
	const auto is_flow = [p_neighbours](int i = 6) { return p_neighbours[i].GetID() == FlowID; };
	const auto is_liquid = [p_neighbours, &is_source, &is_flow](int i = 6) { return is_source(i) || is_flow(i); };
	const auto is_wall = [p_neighbours, &is_liquid](int i = 6) {
		return p_neighbours[i].GetID() != Blocks::kAir && !is_liquid(i);
	};
	const auto get_lvl = [p_neighbours, &is_source, &is_flow](int i = 6) -> int8_t {
		return is_source(i) ? FlowDistance : (is_flow(i) ? p_neighbours[i].GetMeta() : -1);
	};
	const auto get_abs_lvl = [p_neighbours, &get_lvl, &is_wall](int i = 6) -> int8_t {
		return is_wall(i) ? FlowDistance : get_lvl(i);
	};

	if (!is_liquid())
		return;

	// Flow -> Source
	/* if (!is_source() && is_source(BlockFaces::kLeft) && is_source(BlockFaces::kRight) && is_source(BlockFaces::kBack)
	&& is_source(BlockFaces::kFront)) { p_out_set_blocks[(*p_out_set_block_count)++] = {6, SourceID}; return;
	} */

	int8_t lvl = get_lvl();

	// Down propagation
	if (!is_liquid(BlockFaces::kTop)) {
		if (!is_liquid(BlockFaces::kLeft) && !is_liquid(BlockFaces::kRight) && !is_liquid(BlockFaces::kBack) &&
		    !is_liquid(BlockFaces::kFront) && !is_source()) {
			p_out_set_blocks[(*p_out_set_block_count)++] = {6, Blocks::kAir};
		} else if (is_flow() && lvl == FlowDistance) {
			p_out_set_blocks[(*p_out_set_block_count)++] = {6, Blocks::kAir};
		}
	}
	if (!is_wall(BlockFaces::kBottom)) {
		p_out_set_blocks[(*p_out_set_block_count)++] = {BlockFaces::kBottom, Block{FlowID, FlowDistance, 0}};
	}

	// Shrink
	if (is_flow() && lvl < FlowDistance) {
		if (lvl >= get_lvl(BlockFaces::kLeft) && lvl >= get_lvl(BlockFaces::kRight) &&
		    lvl >= get_lvl(BlockFaces::kBack) && lvl >= get_lvl(BlockFaces::kFront)) {
			p_out_set_blocks[(*p_out_set_block_count)++] = {6, Blocks::kAir};
			if (is_flow(BlockFaces::kLeft))
				p_out_set_blocks[(*p_out_set_block_count)++] = {BlockFaces::kLeft, Blocks::kAir};
			if (is_flow(BlockFaces::kRight))
				p_out_set_blocks[(*p_out_set_block_count)++] = {BlockFaces::kRight, Blocks::kAir};
			if (is_flow(BlockFaces::kBack))
				p_out_set_blocks[(*p_out_set_block_count)++] = {BlockFaces::kBack, Blocks::kAir};
			if (is_flow(BlockFaces::kFront))
				p_out_set_blocks[(*p_out_set_block_count)++] = {BlockFaces::kFront, Blocks::kAir};
			return;
		}
	}

	// Spread
	if (!is_wall(BlockFaces::kBottom))
		return;
	if (lvl > 0) {
		if (!is_wall(BlockFaces::kLeft) && lvl > get_lvl(BlockFaces::kLeft) + 1)
			p_out_set_blocks[(*p_out_set_block_count)++] = {BlockFaces::kLeft, Block{FlowID, BlockMeta(lvl - 1), 0}};
		if (!is_wall(BlockFaces::kRight) && lvl > get_lvl(BlockFaces::kRight) + 1)
			p_out_set_blocks[(*p_out_set_block_count)++] = {BlockFaces::kRight, Block{FlowID, BlockMeta(lvl - 1), 0}};
		if (!is_wall(BlockFaces::kBack) && lvl > get_lvl(BlockFaces::kBack) + 1)
			p_out_set_blocks[(*p_out_set_block_count)++] = {BlockFaces::kBack, Block{FlowID, BlockMeta(lvl - 1), 0}};
		if (!is_wall(BlockFaces::kFront) && lvl > get_lvl(BlockFaces::kFront) + 1)
			p_out_set_blocks[(*p_out_set_block_count)++] = {BlockFaces::kFront, Block{FlowID, BlockMeta(lvl - 1), 0}};
	}
}

template <BlockID SourceID, BlockID FlowID, uint32_t FlowDistance, uint32_t FlowTickInterval>
inline static BlockEvent kLiquidEvent = {
    FlowTickInterval,
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
    on_liquid_update<SourceID, FlowID, FlowDistance>,
};

} // namespace hc::block
