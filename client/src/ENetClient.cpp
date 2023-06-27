#include <client/ENetClient.hpp>
#include <spdlog/spdlog.h>

namespace hc::client {

std::shared_ptr<ENetClient> ENetClient::Create(const std::shared_ptr<World> &world_ptr, const char *host_str,
                                               uint16_t port, uint32_t timeout) {
	ENetHost *host;
	host = enet_host_create(nullptr /* create a client host */, 1 /* only allow 1 outgoing connection */,
	                        2 /* allow up 2 channels to be used, 0 and 1 */,
	                        0 /* assume any amount of incoming bandwidth */,
	                        0 /* assume any amount of outgoing bandwidth */);
	if (host == nullptr)
		return nullptr;
	std::shared_ptr<ENetClient> ret = std::make_shared<ENetClient>();
	ret->m_world_ptr = world_ptr;
	world_ptr->m_client_weak_ptr = ret->weak_from_this();

	ret->m_host = host;
	ret->m_connect_future = std::async(&ENetClient::connect, ret.get(), std::string{host_str}, port, timeout);
	return ret;
}

bool ENetClient::IsConnected() {
	if (m_connect_future.valid() && m_connect_future.wait_for(std::chrono::seconds(0)) == std::future_status::ready)
		m_peer = m_connect_future.get();
	return m_peer;
}

ENetPeer *ENetClient::connect(std::string &&host, uint16_t port, uint32_t timeout) {
	ENetAddress address;
	ENetEvent event;
	enet_address_set_host(&address, host.c_str());
	address.port = port;
	/* Initiate the connection, allocating the two channels 0 and 1. */
	ENetPeer *ret = enet_host_connect(m_host, &address, 2, 0);
	if (ret == nullptr) {
		spdlog::error("No available peers for initiating an ENet connection.");
		return nullptr;
	}
	/* Wait up to 5 seconds for the connection attempt to succeed. */
	if (enet_host_service(m_host, &event, timeout) > 0 && event.type == ENET_EVENT_TYPE_CONNECT) {
		spdlog::info("Connection to {}:{} succeeded.", host, port);
		return ret;
	} else {
		/* Either the 5 seconds are up or a disconnect event was */
		/* received. Reset the peer in the event the 5 seconds   */
		/* had run out without any significant event.            */
		enet_peer_reset(ret);
		spdlog::error("Connection to {}:{} failed.", host, port);
		return nullptr;
	}
}

} // namespace hc::client