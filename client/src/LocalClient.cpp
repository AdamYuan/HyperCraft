#include <client/LocalClient.hpp>

#include <client/DefaultTerrain.hpp>

namespace hc::client {

void LocalClient::tick_thread_func() {
	while (m_thread_running.load(std::memory_order_acquire)) {
		std::this_thread::sleep_for(std::chrono::milliseconds(50));
		m_world_ptr->NextTick();
	}
}

std::shared_ptr<LocalClient> LocalClient::Create(const std::shared_ptr<World> &world_ptr,
                                                 const char *database_filename) {
	std::shared_ptr<LocalClient> ret = std::make_shared<LocalClient>();
	ret->m_world_ptr = world_ptr;
	world_ptr->m_client_weak_ptr = ret->weak_from_this();

	if (!(ret->m_world_database = WorldDatabase::Create(database_filename)))
		return nullptr;
	if (!(ret->m_terrain = DefaultTerrain::Create(12314524)))
		return nullptr;

	ret->m_tick_thread = std::thread(&LocalClient::tick_thread_func, ret.get());
	return ret;
}

LocalClient::~LocalClient() {
	m_thread_running.store(false, std::memory_order_release);
	m_tick_thread.join();
}

void LocalClient::LoadChunks(std::span<const ChunkPos3> chunk_pos_s) {
	auto chunk_entries = m_world_database->GetChunks(chunk_pos_s);
}

void LocalClient::SetChunkBlocks(ChunkPos3 chunk_pos, std::span<const ChunkBlockEntry> blocks) {
	m_world_database->SetBlocks(chunk_pos, blocks);
}

void LocalClient::SetChunkSunlights(ChunkPos3 chunk_pos, std::span<const ChunkSunlightEntry> sunlights) {
	m_world_database->SetSunlights(chunk_pos, sunlights);
}

} // namespace hc::client