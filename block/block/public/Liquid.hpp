#pragma once

#include <block/Block.hpp>
#include <cstdio>

namespace hc::block {

template <BlockID SourceID, BlockID FlowID, uint32_t FlowDistance>
inline static void on_liquid_update(const Block *p_neighbours, Block *p_out_set_blocks, BlockExtraGetBlockFunc) {
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

	if (!is_liquid())
		return;

	// Flow -> Source
	/* if (!is_source() && is_source(BlockFaces::kLeft) && is_source(BlockFaces::kRight) && is_source(BlockFaces::kBack)
	&& is_source(BlockFaces::kFront)) { p_out_set_blocks[(*p_out_set_block_count)++] = {6, SourceID}; return;
	} */

	int8_t lvl = get_lvl();

	// Down Shrink
	if (!is_liquid(BlockFaces::kTop)) {
		if (!is_liquid(BlockFaces::kLeft) && !is_liquid(BlockFaces::kRight) && !is_liquid(BlockFaces::kBack) &&
		    !is_liquid(BlockFaces::kFront) && !is_source()) {
			p_out_set_blocks[6] = Blocks::kAir;
			return;
		} else if (is_flow() && lvl == FlowDistance) {
			p_out_set_blocks[6] = Blocks::kAir;
			return;
		}
	}

	// Shrink
	int8_t lvl_left = get_lvl(BlockFaces::kLeft), lvl_right = get_lvl(BlockFaces::kRight),
	       lvl_back = get_lvl(BlockFaces::kBack), lvl_front = get_lvl(BlockFaces::kFront);
	if (is_flow() && lvl < FlowDistance && (~lvl)) {
		if (lvl >= lvl_left && lvl >= lvl_right && lvl >= lvl_back && lvl >= lvl_front) {
			Block new_blk = lvl == 0 ? Blocks::kAir : Block{FlowID, BlockMeta(lvl - 1), 0};
			p_out_set_blocks[6] = new_blk;
			return;
		}
	}

	if (is_wall(BlockFaces::kBottom)) {
		// Spread
		if (lvl > 0) {
			if (!is_wall(BlockFaces::kLeft) && lvl > lvl_left + 1)
				p_out_set_blocks[BlockFaces::kLeft] = Block{FlowID, BlockMeta(lvl - 1), 0};
			if (!is_wall(BlockFaces::kRight) && lvl > lvl_right + 1)
				p_out_set_blocks[BlockFaces::kRight] = Block{FlowID, BlockMeta(lvl - 1), 0};
			if (!is_wall(BlockFaces::kBack) && lvl > lvl_back + 1)
				p_out_set_blocks[BlockFaces::kBack] = Block{FlowID, BlockMeta(lvl - 1), 0};
			if (!is_wall(BlockFaces::kFront) && lvl > lvl_front + 1)
				p_out_set_blocks[BlockFaces::kFront] = Block{FlowID, BlockMeta(lvl - 1), 0};
		}
	} else if (!is_source(BlockFaces::kBottom)) {
		// Down Propagation
		p_out_set_blocks[BlockFaces::kBottom] = Block{FlowID, FlowDistance, 0};
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
