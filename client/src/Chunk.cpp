#include <client/Chunk.hpp>

#include <bitset>
#include <spdlog/spdlog.h>

void Chunk::Mesh::Push(std::vector<Vertex> &&vertices, std::vector<uint16_t> &&indices,
                       const std::chrono::time_point<std::chrono::steady_clock> &starting_time) {
	std::lock_guard lock_guard{m_mutex};
	if (starting_time > m_update_time) {
		m_updated = true;
		m_vertices = std::move(vertices);
		m_indices = std::move(indices);
	}
}

bool Chunk::Mesh::Pop(std::vector<Vertex> *vertices, std::vector<uint16_t> *indices) {
	std::lock_guard lock_guard{m_mutex};
	if (m_updated) {
		m_updated = false;
		*vertices = std::move(m_vertices);
		*indices = std::move(m_indices);

		m_vertices.clear();
		m_vertices.shrink_to_fit();
		m_indices.clear();
		m_indices.shrink_to_fit();

		return true;
	}
	return false;
}
