#include <client/World.hpp>

#include <glm/gtx/string_cast.hpp>
#include <spdlog/spdlog.h>

#include <client/ChunkGenerator.hpp>
#include <client/ChunkMesher.hpp>
#include <client/MeshEraser.hpp>

void World::launch_worker_threads() {
	m_worker_threads.resize(std::thread::hardware_concurrency());
	for (auto &i : m_worker_threads)
		i = std::thread(&World::worker_thread_func, this);
}
void World::worker_thread_func() {
	moodycamel::ConsumerToken consumer_token{m_workers};
	while (m_worker_threads_running.load(std::memory_order_acquire)) {
		std::unique_ptr<WorkerBase> worker{};

		if (m_workers.wait_dequeue_timed(consumer_token, worker, std::chrono::milliseconds(10))) {
			worker->Run();
			worker = nullptr;
		}
	}
}

void World::Join() {
	for (const auto &i : m_chunks)
		i.second->SetMeshFinalize();
	m_worker_threads_running.store(false, std::memory_order_release);
	for (auto &i : m_worker_threads)
		i.join();
}

#include "WorldLoadingList.inl"

void World::Update(const glm::vec3 &position) {
	constexpr int16_t kR = 10;

	static glm::i16vec3 last_chunk_pos = {INT16_MAX, INT16_MAX, INT16_MAX};
	glm::i16vec3 current_chunk_pos =
	    (glm::i16vec3)position / (int16_t)Chunk::kSize - (glm::i16vec3)glm::lessThan(position, {0.0f, 0.0f, 0.0f});

	// if (last_chunk_pos.x == INT16_MAX) {
	if (current_chunk_pos != last_chunk_pos) {
		last_chunk_pos = current_chunk_pos;
		spdlog::info("current_chunk_pos = {}", glm::to_string(current_chunk_pos));

		std::vector<std::unique_ptr<WorkerBase>> new_workers, new_nei_workers;
		std::vector<std::unique_ptr<MeshHandle<ChunkMeshVertex, uint16_t, ChunkMeshInfo>>> erased_meshes;
		erased_meshes.reserve(kR * kR * 4);

		for (auto it = m_chunks.begin(); it != m_chunks.end();) {

			if (glm::distance((glm::vec3)current_chunk_pos, (glm::vec3)it->first) > kR + 2) {
				auto meshes = it->second->MoveMeshes();
				if (!meshes.empty())
					erased_meshes.insert(erased_meshes.end(), std::make_move_iterator(meshes.begin()),
					                     std::make_move_iterator(meshes.end()));
				it = m_chunks.erase(it);
			} else {
				if (!it->second->IsMeshed())
					new_nei_workers.push_back(ChunkMesher::Create(it->second));
				++it;
			}
		}
		if (!erased_meshes.empty())
			new_workers.push_back(
			    MeshEraser<ChunkMeshVertex, uint16_t, ChunkMeshInfo>::Create(std::move(erased_meshes)));

		for (const ChunkPos3 *i = kWorldLoadingList; i != kWorldLoadingRadiusEnd[kR]; ++i) {
			ChunkPos3 pos = current_chunk_pos + *i;
			if (!FindChunk(pos))
				new_workers.push_back(ChunkGenerator::Create(PushChunk(pos)));
		}

		PushWorkers(std::move(new_workers));
		PushWorkers(std::move(new_nei_workers));
	}
}

std::shared_ptr<Chunk> World::PushChunk(const ChunkPos3 &position) {
	{ // If exists, return the existing one
		const std::shared_ptr<Chunk> &ret = FindChunk(position);
		if (ret)
			return ret;
	}
	// else, create a new chunk
	std::shared_ptr<Chunk> ret = Chunk::Create(weak_from_this(), position);
	m_chunks[position] = ret;

	// assign chunk neighbours
	for (uint32_t i = 0; i < 26; ++i) {
		ChunkPos3 dp;
		Chunk::NeighbourIndex2CmpXYZ(i, glm::value_ptr(dp));

		const std::shared_ptr<Chunk> &nei = FindChunk(position + dp);
		ret->SetNeighbour(i, nei);
		if (nei)
			nei->SetNeighbour(Chunk::CmpXYZ2NeighbourIndex(-dp.x, -dp.y, -dp.z), ret);
	}
	return ret;
}

std::shared_ptr<Chunk> World::FindChunk(const ChunkPos3 &position) const {
	auto it = m_chunks.find(position);
	return it == m_chunks.end() ? nullptr : it->second;
}
