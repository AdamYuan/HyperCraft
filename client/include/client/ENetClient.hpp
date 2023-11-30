#ifndef HYPERCRAFT_CLIENT_ENET_CLIENT_HPP
#define HYPERCRAFT_CLIENT_ENET_CLIENT_HPP

#include <client/ClientBase.hpp>

#include <enet/enet.h>

#include <future>

namespace hc::client {

class ENetClient final : public ClientBase, public std::enable_shared_from_this<ENetClient> {
private:
	ENetHost *m_host{};
	ENetPeer *m_peer{nullptr};

	std::future<ENetPeer *> m_connect_future;
	ENetPeer *connect(std::string &&host, uint16_t port, uint32_t timeout);

public:
	~ENetClient() final = default;
	static std::shared_ptr<ENetClient> Create(const std::shared_ptr<World> &world_ptr, const char *host, uint16_t port,
	                                          uint32_t timeout = 5000);

	bool IsConnected() final;
	void Disconnect() {
		if (m_peer) {
			enet_peer_disconnect(m_peer, ENET_EVENT_TYPE_DISCONNECT);
			m_peer = nullptr;
		}
	}

	void LoadChunks(std::span<const ChunkPos3> chunk_pos_s) final {}
	void SetChunkBlocks(ChunkPos3 chunk_pos, std::span<const ChunkSetBlockEntry> blocks) final {}
	void SetChunkSunlights(ChunkPos3 chunk_pos, std::span<const ChunkSetSunlightEntry> sunlights) final {}
};

} // namespace hc::client

#endif
