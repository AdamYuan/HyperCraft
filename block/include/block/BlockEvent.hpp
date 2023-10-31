#pragma once

#include "BlockFace.hpp"

namespace hc::block {

class Block;

struct BlockUpdateSetBlock; // Forward decl, defined in Block.hpp
using BlockOnUpdateFunc = void (*)(const Block *, BlockUpdateSetBlock *, uint32_t *);

constexpr uint32_t kBlockUpdateMaxNeighbours = 32;
struct BlockEvent {
	uint32_t update_tick_interval{0};

	glm::i8vec3 update_neighbours[kBlockUpdateMaxNeighbours]{};
	uint32_t update_neighbour_count{0};
	BlockOnUpdateFunc on_update_func{nullptr};
};

} // namespace hc::block
