#pragma once

#include "BlockFace.hpp"
#include <functional>

namespace hc::block {

class Block;

using BlockOnUpdateFunc = void (*)(const Block *, Block *, std::function<Block(glm::i8vec3)>);

constexpr uint32_t kBlockUpdateMaxNeighbours = 32;
struct BlockEvent {
	uint32_t update_tick_interval{0};

	glm::i8vec3 update_neighbours[kBlockUpdateMaxNeighbours]{};
	uint32_t update_neighbour_count{0};
	BlockOnUpdateFunc on_update_func{nullptr};
};

} // namespace hc::block
