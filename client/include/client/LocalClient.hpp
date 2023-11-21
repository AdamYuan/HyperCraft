#ifndef HYPERCRAFT_CLIENT_LOCAL_CLIENT_HPP
#define HYPERCRAFT_CLIENT_LOCAL_CLIENT_HPP

#include <atomic>
#include <client/ClientBase.hpp>
#include <common/WorldDatabase.hpp>
#include <thread>

namespace hc::client {

class LocalClient final : public ClientBase, public std::enable_shared_from_this<LocalClient> {
private:
	std::unique_ptr<WorldDatabase> m_world_database;

	std::atomic_bool m_thread_running{true};
	std::thread m_tick_thread;

	void tick_thread_func();

public:
	~LocalClient() final;
	inline bool IsConnected() final { return true; }

	static std::shared_ptr<LocalClient> Create(const std::shared_ptr<World> &world_ptr, const char *database_filename);

	void LoadChunks(std::span<const ChunkPos3> chunk_pos_s) final;
	void SetChunkBlocks(ChunkPos3 chunk_pos, std::span<const ChunkBlockEntry> blocks) final;
	void SetChunkSunlights(ChunkPos3 chunk_pos, std::span<const ChunkSunlightEntry> sunlights) final;
};

} // namespace hc::client

#endif
