#ifndef CUBECRAFT3_SERVER_ENET_SERVER_HPP
#define CUBECRAFT3_SERVER_ENET_SERVER_HPP

#include <common/WorldDatabase.hpp>
#include <thread>
#include <enet/enet.h>

class ENetServer {
private:
	ENetHost *m_host;
	std::shared_ptr<WorldDatabase> m_level_db_ptr;

	std::thread m_event_thread;
	bool m_thread_run = true;

	void event_thread_func();

public:
	static std::shared_ptr<ENetServer> Create(const std::shared_ptr<WorldDatabase> &level_db, uint16_t port);
	~ENetServer();

	void RunShell();
	void Join();

	const std::shared_ptr<WorldDatabase> &GetLevelDBPtr() const { return m_level_db_ptr; }
};

#endif
