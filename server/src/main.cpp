#include <spdlog/spdlog.h>

#include <common/WorldDatabase.hpp>
#include <server/ENetServer.hpp>

constexpr const char *kLevelDBFilename = "level.db";
constexpr uint16_t kPort = 60000;

int main() {
	enet_initialize();
	spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%t] [%l] %v");
	std::shared_ptr<hc::WorldDatabase> level_db = hc::WorldDatabase::Create(kLevelDBFilename);
	if (!level_db) {
		spdlog::error("Failed to open sqlite3 database {}", kLevelDBFilename);
		return EXIT_FAILURE;
	}
	std::shared_ptr<hc::server::ENetServer> server = hc::server::ENetServer::Create(level_db, kPort);
	if (!server) {
		spdlog::error("Failed to open open server in port {}", kPort);
		return EXIT_FAILURE;
	}
	spdlog::info("Welcome to HyperCraft server!! (db: {}, port: {})", kLevelDBFilename, kPort);
	server->RunShell();
	server->Join();
	enet_deinitialize();
	return EXIT_SUCCESS;
}
