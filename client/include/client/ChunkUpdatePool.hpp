#pragma once

#include <client/Chunk.hpp>
#include <cuckoohash_map.hh>
#include <glm/gtx/hash.hpp>
#include <map>

namespace hc::client {

class World;

class ChunkUpdatePool {
private:
	World &m_world;
	libcuckoo::cuckoohash_map<ChunkPos3, std::map<InnerPos3, block::Block, InnerPosCompare>> m_block_updates;
	// libcuckoo::cuckoohash_map<ChunkPos3, std::map<InnerPos2, ChunkSunlightEntry>> m_sunlight_updates;

public:
	inline explicit ChunkUpdatePool(World *p_world) : m_world{*p_world} {}
	inline void SetBlockUpdate(ChunkPos3 chunk_pos, InnerPos3 inner_pos, block::Block block) {
		m_block_updates.uprase_fn(
		    chunk_pos,
		    [inner_pos, block](auto &data, libcuckoo::UpsertContext) {
			    data[inner_pos] = block;
			    return false;
		    },
		    InnerPosCompare{});
	}
	inline std::optional<block::Block> GetBlockUpdate(ChunkPos3 chunk_pos, InnerPos3 inner_pos) {
		std::optional<block::Block> ret = std::nullopt;
		m_block_updates.find_fn(chunk_pos, [inner_pos, &ret](auto &data) {
			auto it = data.find(inner_pos);
			if (it != data.end())
				ret = it->second;
		});
		return ret;
	}
	/* inline void SetHeightUpdate(ChunkPos2 chunk_pos, InnerPos2 inner_pos, BlockPos1 height) {
		m_height_updates.uprase_fn(
		    chunk_pos,
		    [inner_pos, height](auto &data, libcuckoo::UpsertContext) {
			    data[inner_pos] = height;
			    return false;
		    },
		    InnerPosCompare{});
	}
	inline std::optional<BlockPos1> GetHeightUpdate(ChunkPos2 chunk_pos, InnerPos2 inner_pos) {
		std::optional<BlockPos1> ret = std::nullopt;
		m_height_updates.find_fn(chunk_pos, [inner_pos, &ret](auto &data) {
			auto it = data.find(inner_pos);
			if (it != data.end())
				ret = it->second;
		});
		return ret;
	} */
};

} // namespace hc::client
