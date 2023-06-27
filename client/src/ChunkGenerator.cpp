#include <client/ChunkGenerator.hpp>

#include <client/ChunkLighter.hpp>
#include <client/ChunkMesher.hpp>
#include <client/ClientBase.hpp>
#include <client/World.hpp>

#include <spdlog/spdlog.h>

namespace hc::client {

void ChunkGenerator::Run() {
	if (!lock())
		return;

	std::shared_ptr<ClientBase> client_ptr;
	{
		auto world_ptr = m_chunk_ptr->LockWorld();
		if (!world_ptr)
			return;
		client_ptr = world_ptr->LockClient();
		if (!client_ptr)
			return;
	}

	// If client not connected, return
	if (!client_ptr->IsConnected())
		return;

	thread_local static int32_t light_map[kChunkSize * kChunkSize];
	client_ptr->GetTerrain()->Generate(m_chunk_ptr, light_map);

	// set initial light
	// sunlight from light_map
	for (uint32_t y = 0; y < Chunk::kSize; ++y) {
		int32_t cur_height = m_chunk_ptr->GetPosition().y * (int)Chunk::kSize + (int)y;
		for (uint32_t idx = 0; idx < kChunkSize * kChunkSize; ++idx) {
			if (cur_height > light_map[idx])
				m_chunk_ptr->SetLight(y * kChunkSize * kChunkSize + idx, {15, 0});
		}
	}
	// TODO: add light from luminous blocks

	m_chunk_ptr->SetGeneratedFlag();
	/* spdlog::info("Chunk ({}, {}, {}) generated", m_chunk_ptr->GetPosition().x, m_chunk_ptr->GetPosition().y,
	             m_chunk_ptr->GetPosition().z); */

	for (uint32_t i = 0; i < Chunk::kSize * Chunk::kSize * Chunk::kSize; ++i)
		if (m_chunk_ptr->GetBlock(i) != block::Blocks::kAir) {
			try_push_worker(ChunkMesher::TryCreateWithInitialLight(m_chunk_ptr));
			return;
		}
	m_chunk_ptr->SetMeshedFlag();
}

} // namespace hc::client