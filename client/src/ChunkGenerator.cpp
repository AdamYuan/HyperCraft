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

	m_chunk_ptr->SetGeneratedFlag();
	spdlog::info("Chunk ({}, {}, {}) generated", m_chunk_ptr->GetPosition().x, m_chunk_ptr->GetPosition().y,
	             m_chunk_ptr->GetPosition().z);

	for (uint32_t i = 0; i < Chunk::kSize * Chunk::kSize * Chunk::kSize; ++i)
		if (m_chunk_ptr->GetBlock(i) != Blocks::kAir) {
			push_worker(ChunkMesher::Create(m_chunk_ptr));
			return;
		}

	// if there are no blocks, avoid meshing
	m_chunk_ptr->MoveMesh(); // ensure the mesh is removed
	m_chunk_ptr->SetMeshedFlag();
}
