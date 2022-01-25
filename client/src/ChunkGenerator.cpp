#include <client/ChunkGenerator.hpp>

#include <client/ChunkMesher.hpp>

#include <spdlog/spdlog.h>

#include <random>
void ChunkGenerator::Run() {
	if (!lock())
		return;

	if (m_chunk_ptr->GetPosition().y < 0) {
		std::mt19937 gen{std::random_device{}()};
		for (uint32_t i = 0; i < 1000; ++i)
			m_chunk_ptr->SetBlock(gen() % Chunk::kSize, gen() % Chunk::kSize, gen() % Chunk::kSize, gen() % 8);
	} else {
		std::mt19937 gen{std::random_device{}()};
		for (uint32_t i = 0; i < 10; ++i)
			m_chunk_ptr->SetBlock(gen() % Chunk::kSize, gen() % Chunk::kSize, gen() % Chunk::kSize, gen() % 8);
	}

	spdlog::info("Chunk ({}, {}, {}) generated", m_chunk_ptr->GetPosition().x, m_chunk_ptr->GetPosition().y,
	             m_chunk_ptr->GetPosition().z);

	m_chunk_ptr->EnableFlags(Chunk::Flag::kGenerated);
	push_worker(ChunkMesher::Create(m_chunk_ptr));
}
