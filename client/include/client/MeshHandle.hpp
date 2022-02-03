#ifndef CUBECRAFT3_CLIENT_MESH_HPP
#define CUBECRAFT3_CLIENT_MESH_HPP

#include <memory>
#include <vk_mem_alloc.h>

#include <client/MeshCluster.hpp>

template <typename Vertex, typename Index, typename Info> class MeshHandle {
private:
	std::shared_ptr<MeshCluster<Vertex, Index, Info>> m_cluster_ptr;
	VmaVirtualAllocation m_vertices_allocation{VK_NULL_HANDLE}, m_indices_allocation{VK_NULL_HANDLE};
	uint32_t m_first_index{UINT32_MAX}; // used as a key for searching mesh in the cluster

public:
	inline static std::unique_ptr<MeshHandle>
	Create(const std::shared_ptr<MeshCluster<Vertex, Index, Info>> &cluster_ptr, const std::vector<Vertex> &vertices,
	       const std::vector<Index> &indices, const Info &info) {
		auto ret = std::make_unique<MeshHandle>();
		ret->m_cluster_ptr = cluster_ptr;
		VkDeviceSize vertices_offset, indices_offset;
		{
			VmaVirtualAllocationCreateInfo allocation_info = {};
			allocation_info.flags = VMA_VIRTUAL_ALLOCATION_CREATE_STRATEGY_MIN_MEMORY_BIT;

			allocation_info.alignment = sizeof(Vertex);
			allocation_info.size = vertices.size() * sizeof(Vertex);
			{
				std::scoped_lock lock{cluster_ptr->m_vertices_virtual_block_mutex};
				if (vmaVirtualAllocate(cluster_ptr->m_vertices_virtual_block, &allocation_info,
				                       &ret->m_vertices_allocation, &vertices_offset) != VK_SUCCESS) {
					return nullptr;
				}
			}

			allocation_info.alignment = sizeof(Index);
			allocation_info.size = indices.size() * sizeof(Index);
			{
				std::scoped_lock lock{cluster_ptr->m_indices_virtual_block_mutex};
				if (vmaVirtualAllocate(cluster_ptr->m_indices_virtual_block, &allocation_info,
				                       &ret->m_indices_allocation, &indices_offset) != VK_SUCCESS) {
					return nullptr;
				}
			}
		}
		ret->m_first_index = cluster_ptr->insert_mesh(vertices, vertices_offset, indices, indices_offset, info);
		return ret;
	}

	~MeshHandle() {
		if (~m_first_index)
			m_cluster_ptr->erase_mesh(m_first_index);
		if (m_vertices_allocation) {
			std::scoped_lock lock{m_cluster_ptr->m_vertices_virtual_block_mutex};
			vmaVirtualFree(m_cluster_ptr->m_vertices_virtual_block, m_vertices_allocation);
		}
		if (m_indices_allocation) {
			std::scoped_lock lock{m_cluster_ptr->m_indices_virtual_block_mutex};
			vmaVirtualFree(m_cluster_ptr->m_indices_virtual_block, m_indices_allocation);
		}
	}
};

#endif
