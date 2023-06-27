#include <server/ENetServer.hpp>
#include <spdlog/spdlog.h>

#include <iostream>

namespace hc::server {

std::shared_ptr<ENetServer> ENetServer::Create(const std::shared_ptr<WorldDatabase> &level_db, uint16_t port) {
	ENetAddress address;
	address.host = ENET_HOST_ANY;
	address.port = port;
	ENetHost *host;
	host = enet_host_create(
	    &address /* the address to bind the server host to */,
	    32 /* allow up to 32 clients and/or outgoing connections */, 2 /* allow up to 2 channels to be used, 0 and 1 */,
	    0 /* assume any amount of incoming bandwidth */, 0 /* assume any amount of outgoing bandwidth */);
	if (host == nullptr)
		return nullptr;
	std::shared_ptr<ENetServer> ret = std::make_shared<ENetServer>();
	ret->m_level_db_ptr = level_db;
	ret->m_host = host;
	ret->m_event_thread = std::thread(&ENetServer::event_thread_func, ret.get());
	return ret;
}

void ENetServer::Join() {
	m_thread_run = false;
	m_event_thread.join();
}

ENetServer::~ENetServer() { enet_host_destroy(m_host); }

void ENetServer::RunShell() {
	std::string line;
	printf("> ");
	fflush(stdout);
	while (std::getline(std::cin, line)) {
		if (!line.empty())
			printf("%s\n", line.c_str());
		printf("> ");
		fflush(stdout);
	}
}

void ENetServer::event_thread_func() {
	spdlog::info("Enter event thread");
	ENetEvent event;
	while (m_thread_run) {
		if (enet_host_service(m_host, &event, 0) > 0) {
			switch (event.type) {
			case ENET_EVENT_TYPE_CONNECT:
				spdlog::info("A new client connected from {}:{}.", event.peer->address.host, event.peer->address.port);
				/* Store any relevant client information here. */
				event.peer->data = (void *)"Client information";
				break;
			case ENET_EVENT_TYPE_RECEIVE:
				spdlog::info("A packet of length {} containing {} was received from {} on channel {}.",
				             event.packet->dataLength, event.packet->data, (char *)event.peer->data, event.channelID);
				/* Clean up the packet now that we're done using it. */
				enet_packet_destroy(event.packet);

				break;

			case ENET_EVENT_TYPE_DISCONNECT:
				spdlog::info("{} disconnected.", (char *)event.peer->data);
				/* Reset the peer's client information. */
				event.peer->data = nullptr;
			}
		}
	}
	spdlog::info("Quit event thread");
}

} // namespace hc::server
