#pragma once

#include <block/Block.hpp>

namespace hc::block {

template <BlockID SourceID, BlockID FlowID, uint32_t FlowDistance>
inline static void on_liquid_update(const Block *p_neighbours, BlockUpdateSetBlock *p_out_set_blocks,
                                    uint32_t *p_out_set_block_count) {
	if (p_neighbours[6].GetID() == SourceID || p_neighbours[6].GetID() == FlowID)
		if (p_neighbours[BlockFaces::kBottom].GetID() == Blocks::kAir) {
			p_out_set_blocks[(*p_out_set_block_count)++] = {BlockFaces::kBottom, Block{SourceID}};
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
