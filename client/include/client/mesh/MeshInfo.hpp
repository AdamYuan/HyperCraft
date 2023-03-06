#ifndef CUBECRAFT3_CLIENT_MESH_MESHINFO_HPP
#define CUBECRAFT3_CLIENT_MESH_MESHINFO_HPP

#include <cinttypes>

// Mesh information
template <typename Info> struct MeshInfo {
	uint32_t m_index_count, m_first_index, m_vertex_offset;
	Info m_info;
};

#endif
