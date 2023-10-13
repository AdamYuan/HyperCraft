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
};

} // namespace hc::client
