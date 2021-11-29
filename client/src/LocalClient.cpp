#include <client/LocalClient.hpp>

std::shared_ptr<LocalClient> LocalClient::Create(const std::shared_ptr<World> &world_ptr,
                                                 const std::shared_ptr<LevelDB> &level_db_ptr) {
	std::shared_ptr<LocalClient> ret = std::make_shared<LocalClient>(world_ptr, level_db_ptr);
	return ret;
}
