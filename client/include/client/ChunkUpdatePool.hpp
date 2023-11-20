#pragma once

#include <client/Chunk.hpp>
#include <client/ChunkUpdate.hpp>
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
	libcuckoo::cuckoohash_map<ChunkPos3, std::map<InnerIndex3, block::Block>> m_block_updates;
	libcuckoo::cuckoohash_map<ChunkPos3, std::map<InnerIndex2, InnerPos1>> m_sunlight_updates;

public:
	inline explicit ChunkUpdatePool(World *p_world) : m_world{*p_world} {}
	void SetBlockUpdate(ChunkPos3 chunk_pos, ChunkSetBlock set_block, bool active);
	void SetBlockUpdateBulk(ChunkPos3 chunk_pos, std::span<const ChunkSetBlock> set_blocks, bool active);
	inline std::optional<block::Block> GetBlockUpdate(ChunkPos3 chunk_pos, InnerPos3 inner_pos) const {
		std::optional<block::Block> ret = std::nullopt;
		InnerIndex3 idx = InnerIndex3FromPos(inner_pos);
		m_block_updates.find_fn(chunk_pos, [idx, &ret](auto &data) {
			auto it = data.find(idx);
			if (it != data.end())
				ret = it->second;
		});
		return ret;
	}
	inline auto GetBlockUpdateBulk(ChunkPos3 chunk_pos) const {
		std::map<InnerIndex3, block::Block> ret;
		m_block_updates.find_fn(chunk_pos, [&ret](auto &data) { ret = data; });
		return ret;
	}
	inline void ApplyBlockUpdates(const std::shared_ptr<Chunk> &chunk_ptr) {
		m_block_updates.find_fn(chunk_ptr->GetPosition(), [&chunk_ptr](auto &data) {
			for (auto it = data.begin(); it != data.end();) {
				if (chunk_ptr->GetBlock(it->first) != it->second) {
					chunk_ptr->SetBlock(it->first, it->second);
					++it;
				} else
					it = data.erase(it);
			}
		});
	}
	void SetSunlightUpdate(ChunkPos3 chunk_pos, ChunkSetSunlight set_sunlight, bool active);
	void SetSunlightUpdateBulk(ChunkPos3 chunk_pos, std::span<const ChunkSetSunlight> set_sunlights, bool active);

	inline std::optional<InnerPos1> GetSunlightUpdate(ChunkPos3 chunk_pos, InnerPos2 inner_pos) const {
		std::optional<InnerPos1> ret = std::nullopt;
		InnerIndex2 idx = InnerIndex2FromPos(inner_pos);
		m_sunlight_updates.find_fn(chunk_pos, [idx, &ret](auto &data) {
			auto it = data.find(idx);
			if (it != data.end())
				ret = it->second;
		});
		return ret;
	}
	inline auto GetSunlightUpdateBulk(ChunkPos3 chunk_pos) const {
		std::map<InnerIndex2, InnerPos1> ret;
		m_sunlight_updates.find_fn(chunk_pos, [&ret](auto &data) { ret = data; });
		return ret;
	}
	inline void ApplySunlightUpdates(const std::shared_ptr<Chunk> &chunk_ptr) {
		m_sunlight_updates.find_fn(chunk_ptr->GetPosition(), [&chunk_ptr](auto &data) {
			for (auto it = data.begin(); it != data.end();) {
				if (chunk_ptr->GetSunlightHeight(it->first) != it->second) {
					chunk_ptr->SetSunlightHeight(it->first, it->second);
					++it;
				} else
					it = data.erase(it);
			}
		});
	}
};

} // namespace hc::client
