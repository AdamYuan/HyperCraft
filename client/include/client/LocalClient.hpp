#ifndef CUBECRAFT3_CLIENT_LOCAL_CLIENT_HPP
#define CUBECRAFT3_CLIENT_LOCAL_CLIENT_HPP

#include <client/ClientBase.hpp>
#include <common/WorldDatabase.hpp>

class LocalClient : public ClientBase {
private:
	std::shared_ptr<WorldDatabase> m_world_database_ptr;

public:
	inline explicit LocalClient(const std::shared_ptr<World> &world_ptr,
	                            const std::shared_ptr<WorldDatabase> &level_db_ptr)
	    : ClientBase{world_ptr}, m_world_database_ptr{level_db_ptr} {}
	~LocalClient() override = default;
	inline bool IsConnected() const override { return true; }

	static std::shared_ptr<LocalClient> Create(const std::shared_ptr<World> &world_ptr,
	                                           const std::shared_ptr<WorldDatabase> &level_db_ptr);

	inline const std::shared_ptr<WorldDatabase> &GetWorldDatabasePtr() const { return m_world_database_ptr; }
};

#endif
