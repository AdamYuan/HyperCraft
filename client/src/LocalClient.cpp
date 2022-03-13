#include <client/LocalClient.hpp>

#include <client/DefaultTerrain.hpp>

std::shared_ptr<LocalClient> LocalClient::Create(const std::shared_ptr<World> &world_ptr,
                                                 const char *database_filename) {
	std::shared_ptr<LocalClient> ret = std::make_shared<LocalClient>();
	ret->m_world_ptr = world_ptr;
	world_ptr->m_client_weak_ptr = ret->weak_from_this();

	if (!(ret->m_world_database = WorldDatabase::Create(database_filename)))
		return nullptr;
	if (!(ret->m_terrain = DefaultTerrain::Create(1212313)))
		return nullptr;
	return ret;
}
