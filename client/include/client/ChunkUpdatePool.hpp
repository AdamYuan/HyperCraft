#pragma once

#include <client/Chunk.hpp>
#include <cuckoohash_map.hh>
#include <glm/gtx/hash.hpp>
#include <map>
#include <optional>
#include <span>

namespace hc::client {

class World;

class ChunkUpdatePool {
private:
	World &m_world;
	libcuckoo::cuckoohash_map<ChunkPos3, std::map<InnerPos3, block::Block, InnerPosCompare>> m_block_updates;
	libcuckoo::cuckoohash_map<ChunkPos3, std::map<InnerPos2, InnerPos1, InnerPosCompare>> m_sunlight_updates;

public:
	inline explicit ChunkUpdatePool(World *p_world) : m_world{*p_world} {}
	void SetBlockUpdate(ChunkPos3 chunk_pos, InnerPos3 inner_pos, block::Block block, bool active);
	void SetBlockUpdateBulk(ChunkPos3 chunk_pos, std::span<const std::pair<InnerPos3, block::Block>> blocks,
	                        bool active);
	inline std::optional<block::Block> GetBlockUpdate(ChunkPos3 chunk_pos, InnerPos3 inner_pos) const {
		std::optional<block::Block> ret = std::nullopt;
		m_block_updates.find_fn(chunk_pos, [inner_pos, &ret](auto &data) {
			auto it = data.find(inner_pos);
			if (it != data.end())
				ret = it->second;
		});
		return ret;
	}
	inline std::map<InnerPos3, block::Block, InnerPosCompare> GetBlockUpdateBulk(ChunkPos3 chunk_pos) const {
		std::map<InnerPos3, block::Block, InnerPosCompare> ret{InnerPosCompare{}};
		m_block_updates.find_fn(chunk_pos, [&ret](auto &data) { ret = data; });
		return ret;
	}
	void SetSunlightUpdate(ChunkPos3 chunk_pos, InnerPos2 inner_pos, InnerPos1 sunlight_height);
	void SetSunlightUpdateBulk(ChunkPos3 chunk_pos, std::span<const std::pair<InnerPos2, InnerPos1>> sunlights);

	inline std::optional<InnerPos1> GetSunlightUpdate(ChunkPos3 chunk_pos, InnerPos2 inner_pos) const {
		std::optional<InnerPos1> ret = std::nullopt;
		m_sunlight_updates.find_fn(chunk_pos, [inner_pos, &ret](auto &data) {
			auto it = data.find(inner_pos);
			if (it != data.end())
				ret = it->second;
		});
		return ret;
	}
	inline std::map<InnerPos2, InnerPos1, InnerPosCompare> GetSunlightUpdateBulk(ChunkPos3 chunk_pos) const {
		std::map<InnerPos2, InnerPos1, InnerPosCompare> ret{InnerPosCompare{}};
		m_sunlight_updates.find_fn(chunk_pos, [&ret](auto &data) { ret = data; });
		return ret;
	}
};

} // namespace hc::client
