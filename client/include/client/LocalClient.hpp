#ifndef HYPERCRAFT_CLIENT_LOCAL_CLIENT_HPP
#define HYPERCRAFT_CLIENT_LOCAL_CLIENT_HPP

#include <client/ClientBase.hpp>
#include <common/WorldDatabase.hpp>
#include <thread>
#include <atomic>

namespace hc::client {

class LocalClient : public ClientBase, public std::enable_shared_from_this<LocalClient> {
private:
	std::unique_ptr<WorldDatabase> m_world_database;

	std::atomic_bool m_thread_running{true};
	std::thread m_tick_thread;

	void tick_thread_func();

public:
	~LocalClient() override;
	inline bool IsConnected() override { return true; }

	static std::shared_ptr<LocalClient> Create(const std::shared_ptr<World> &world_ptr, const char *database_filename);
};

} // namespace hc::client

#endif
