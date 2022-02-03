#ifndef CUBECRAFT3_CLIENT_ENET_CLIENT_HPP
#define CUBECRAFT3_CLIENT_ENET_CLIENT_HPP

#include <client/ClientBase.hpp>

#include <enet/enet.h>

#include <future>

class ENetClient : public ClientBase, public std::enable_shared_from_this<ENetClient> {
private:
	ENetHost *m_host{};
	ENetPeer *m_peer{nullptr};

	std::future<ENetPeer *> m_connect_future;
	ENetPeer *connect(std::string &&host, uint16_t port, uint32_t timeout);

public:
	~ENetClient() override = default;
	static std::shared_ptr<ENetClient> Create(const std::shared_ptr<World> &world_ptr, const char *host, uint16_t port,
	                                          uint32_t timeout = 5000);

	bool IsConnected() override;
	void Disconnect() {
		if (m_peer) {
			enet_peer_disconnect(m_peer, ENET_EVENT_TYPE_DISCONNECT);
			m_peer = nullptr;
		}
	}
};

#endif
