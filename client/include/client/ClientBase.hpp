#ifndef HYPERCRAFT_CLIENT_CLIENT_BASE_HPP
#define HYPERCRAFT_CLIENT_CLIENT_BASE_HPP

#include <client/TerrainBase.hpp>
#include <client/World.hpp>

#include <spdlog/spdlog.h>

#include <future>

namespace hc::client {

struct ClientChunkBlock {
	InnerIndex3 index{};
	block::Block block;
};
struct ClientChunkSunlight {
	InnerIndex2 index{};
	InnerPos1 sunlight{};
};
struct ClientChunk {
	std::vector<ClientChunkBlock> blocks;
	std::vector<ClientChunkSunlight> sunlights;
};

class ClientBase {
protected:
	std::shared_ptr<World> m_world_ptr;
	std::unique_ptr<TerrainBase> m_terrain;

public:
	virtual ~ClientBase() = default;
	inline const std::shared_ptr<World> &GetWorldPtr() const { return m_world_ptr; }
	inline const std::unique_ptr<TerrainBase> &GetTerrain() const { return m_terrain; }
	virtual bool IsConnected() = 0;
	virtual std::vector<std::future<ClientChunk>> LoadChunks(std::span<const ChunkPos3> chunk_pos_s) = 0;
	virtual void SetChunkBlocks(ChunkPos3 chunk_pos, std::span<const ClientChunkBlock> blocks) = 0;
	virtual void SetChunkSunlights(ChunkPos3 chunk_pos, std::span<const ClientChunkSunlight> sunlights) = 0;
};

} // namespace hc::client

#endif
