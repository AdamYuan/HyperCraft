#ifndef CUBECRAFT3_CLIENT_LOCAL_CLIENT_HPP
#define CUBECRAFT3_CLIENT_LOCAL_CLIENT_HPP

#include <client/ClientBase.hpp>
#include <common/WorldDatabase.hpp>

class LocalClient : public ClientBase, public std::enable_shared_from_this<LocalClient> {
private:
	std::unique_ptr<WorldDatabase> m_world_database;

public:
	~LocalClient() override = default;
	inline bool IsConnected() override { return true; }

	static std::shared_ptr<LocalClient> Create(const std::shared_ptr<World> &world_ptr, const char *database_filename);
};

#endif
