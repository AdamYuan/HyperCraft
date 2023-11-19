#pragma once

#include <block/Block.hpp>
#include <cinttypes>
#include <common/Position.hpp>

namespace hc::client {

enum class ChunkUpdateType { kLocal, kRemote };

struct ChunkSetBlock {
	InnerIndex3 index{};
	block::Block block;
	ChunkUpdateType type{};
};

struct ChunkSetSunlight {
	InnerIndex2 index{};
	InnerPos1 sunlight{};
	ChunkUpdateType type{};
};

} // namespace hc::client
