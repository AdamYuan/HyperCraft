#ifndef HYPERCRAFT_CLIENT_CLIENT_BASE_HPP
#define HYPERCRAFT_CLIENT_CLIENT_BASE_HPP

#include <client/TerrainBase.hpp>
#include <client/World.hpp>

#include <spdlog/spdlog.h>

namespace hc::client {

class ClientBase {
protected:
	std::shared_ptr<World> m_world_ptr;
	std::unique_ptr<TerrainBase> m_terrain;

public:
	virtual ~ClientBase() = default;
	inline const std::shared_ptr<World> &GetWorldPtr() const { return m_world_ptr; }
	inline const std::unique_ptr<TerrainBase> &GetTerrain() const { return m_terrain; }
	virtual bool IsConnected() = 0;
};

} // namespace hc::client

#endif
