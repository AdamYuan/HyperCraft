#include <client/World.hpp>

#include <glm/gtx/string_cast.hpp>
#include <spdlog/spdlog.h>

#include <client/Config.hpp>

namespace hc::client {

World::~World() {
	auto locked_chunks = m_chunks.lock_table();
	for (const auto &i : locked_chunks)
		i.second->SetMeshFinalize();
}

#include "WorldLoadingList.inl"
void World::update() {
	auto chunk_pos = GetCenterChunkPos();
	auto load_radius = GetLoadChunkRadius(), unload_radius = GetUnloadChunkRadius();

	printf("update\n");

	std::vector<ChunkPos3> generate_chunk_pos_vec;
	{
		auto locked_chunks = m_chunks.lock_table();
		for (auto it = locked_chunks.begin(); it != locked_chunks.end();) {
			if (glm::distance((glm::vec3)chunk_pos, (glm::vec3)it->first) > (float)unload_radius)
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

	for (const auto &pos : generate_chunk_pos_vec)
		m_chunk_task_pool.Push<ChunkTaskType::kGenerate>(pos);
}

std::shared_ptr<Chunk> World::FindChunk(const ChunkPos3 &position) const {
	std::shared_ptr<Chunk> ret = nullptr;
	m_chunks.find_fn(position, [&ret](const auto &data) { ret = data; });
	return ret;
}

} // namespace hc::client
