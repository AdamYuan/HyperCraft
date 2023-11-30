#include <client/ChunkPool.hpp>

#include <client/ClientBase.hpp>
#include <client/World.hpp>

namespace hc::client {

#include "WorldLoadingList.inl"
void ChunkPool::Update() {
	auto chunk_pos = m_world.GetCenterChunkPos();
	auto load_radius = m_world.GetLoadChunkRadius(), unload_radius = m_world.GetUnloadChunkRadius();

	std::vector<ChunkPos3> generate_chunk_pos_vec;
	{
		auto locked_chunks = m_chunks.lock_table();
		for (auto it = locked_chunks.begin(); it != locked_chunks.end();) {
			if (ChunkPosDistance2(chunk_pos, it->first) > uint32_t(unload_radius * unload_radius))
				it = locked_chunks.erase(it);
			else
				++it;
		}
		for (const ChunkPos3 *i = kWorldLoadingList; i != kWorldLoadingRadiusEnd[load_radius]; ++i) {
			ChunkPos3 pos = chunk_pos + *i;
			if (locked_chunks.find(pos) == locked_chunks.end()) {
				locked_chunks.insert(pos, Chunk::Create(pos));
				generate_chunk_pos_vec.push_back(pos);
			}
		}
	}
	auto client = m_world.LockClient();
	if (client)
		client->LoadChunks(generate_chunk_pos_vec);
}

} // namespace hc::client