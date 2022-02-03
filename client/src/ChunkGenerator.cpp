#include <client/ChunkGenerator.hpp>

#include <client/ChunkMesher.hpp>
#include <client/ClientBase.hpp>
#include <client/World.hpp>

#include <spdlog/spdlog.h>

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

	client_ptr->GetTerrain()->Generate(m_chunk_ptr, nullptr);
	spdlog::info("Chunk ({}, {}, {}) generated", m_chunk_ptr->GetPosition().x, m_chunk_ptr->GetPosition().y,
	             m_chunk_ptr->GetPosition().z);

	m_chunk_ptr->EnableFlags(Chunk::Flag::kGenerated);
	push_worker(ChunkMesher::Create(m_chunk_ptr));
}
