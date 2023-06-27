#ifndef HYPERCRAFT_CLIENT_MESH_MESHINFO_HPP
#define HYPERCRAFT_CLIENT_MESH_MESHINFO_HPP

#include <cinttypes>

namespace hc::client::mesh {

// Mesh information
template <typename Info> struct MeshInfo {
	uint32_t m_index_count, m_first_index, m_vertex_offset;
	Info m_info;
};

}

#endif
