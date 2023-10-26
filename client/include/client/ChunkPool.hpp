#pragma once

#include <client/Chunk.hpp>
#include <cuckoohash_map.hh>

namespace hc::client {

class World;

class ChunkPool {
private:
	World &m_world;
	libcuckoo::cuckoohash_map<ChunkPos3, std::shared_ptr<Chunk>> m_chunks;

public:
	inline explicit ChunkPool(World *p_world) : m_world{*p_world} {}
	void Update();
	inline std::shared_ptr<Chunk> FindRawChunk(const ChunkPos3 &position) const {
		std::shared_ptr<Chunk> ret = nullptr;
		m_chunks.find_fn(position, [&ret](const auto &data) { ret = data; });
		return ret;
	}
	inline std::shared_ptr<Chunk> FindChunk(const ChunkPos3 &position) const {
		auto ret = FindRawChunk(position);
		return ret && ret->IsGenerated() ? ret : nullptr;
	}
};

} // namespace hc::client
