#ifndef CUBECRAFT3_CLIENT_MESH_HPP
#define CUBECRAFT3_CLIENT_MESH_HPP

#include <memory>
#include <vk_mem_alloc.h>

#include <client/MeshCluster.hpp>

template <typename Vertex, typename Index, typename Info> class MeshHandle {
private:
	std::shared_ptr<MeshCluster<Vertex, Index, Info>> m_cluster_ptr;
	VmaVirtualAllocation m_vertex_allocation{VK_NULL_HANDLE}, m_index_allocation{VK_NULL_HANDLE};

	uint64_t m_mesh_id = -1;
	bool m_finalize{false};

	inline void destroy() {
		if (!m_finalize)
			m_cluster_ptr->enqueue_mesh_erase(m_mesh_id);
		if (~m_mesh_id) {
			std::scoped_lock lock{m_cluster_ptr->m_allocation_mutex};
			--m_cluster_ptr->m_allocation_count;
			if (m_vertex_allocation) {
				vmaVirtualFree(m_cluster_ptr->m_vertex_virtual_block, m_vertex_allocation);
				m_vertex_allocation = VK_NULL_HANDLE;
			}
			if (m_index_allocation) {
				vmaVirtualFree(m_cluster_ptr->m_index_virtual_block, m_index_allocation);
				m_index_allocation = VK_NULL_HANDLE;
			}
		}
	}

	template <typename, typename, typename> friend class MeshEraser;

public:
	inline static std::unique_ptr<MeshHandle>
	Create(const std::shared_ptr<MeshCluster<Vertex, Index, Info>> &cluster_ptr, const std::vector<Vertex> &vertices,
	       const std::vector<Index> &indices, const Info &info) {
		auto ret = std::make_unique<MeshHandle>();

		ret->m_cluster_ptr = cluster_ptr;
		VkDeviceSize vertex_offset, index_offset;
		{
			VmaVirtualAllocationCreateInfo vertex_alloc = {}, index_alloc = {};
			vertex_alloc.flags = VMA_VIRTUAL_ALLOCATION_CREATE_STRATEGY_MIN_MEMORY_BIT;
			vertex_alloc.alignment = sizeof(Vertex);
			static_assert((sizeof(Vertex) & (sizeof(Vertex) - 1)) == 0, "sizeof(Vertex) is required to be power of 2");
			vertex_alloc.size = vertices.size() * sizeof(Vertex);

			index_alloc.flags = VMA_VIRTUAL_ALLOCATION_CREATE_STRATEGY_MIN_MEMORY_BIT;
			index_alloc.alignment = sizeof(Index);
			static_assert((sizeof(Index) & (sizeof(Index) - 1)) == 0, "sizeof(Index) is required to be power of 2");
			index_alloc.size = indices.size() * sizeof(Index);

			std::scoped_lock allocation_lock{cluster_ptr->m_allocation_mutex};
			if (cluster_ptr->m_allocation_count >= cluster_ptr->m_max_meshes)
				return nullptr;
			if (vmaVirtualAllocate(cluster_ptr->m_vertex_virtual_block, &vertex_alloc, &ret->m_vertex_allocation,
			                       &vertex_offset) != VK_SUCCESS) {
				return nullptr;
			}
			if (vmaVirtualAllocate(cluster_ptr->m_index_virtual_block, &index_alloc, &ret->m_index_allocation,
			                       &index_offset) != VK_SUCCESS) {
				// Free vertex block if index block allocation failed
				vmaVirtualFree(cluster_ptr->m_vertex_virtual_block, ret->m_vertex_allocation);
				return nullptr;
			}
			++cluster_ptr->m_allocation_count;
		}
		ret->m_mesh_id = cluster_ptr->enqueue_mesh_insert(vertices, vertex_offset, indices, index_offset, info);
		return ret;
	}

	inline void SetFinalize() { m_finalize = true; }

	~MeshHandle() { destroy(); }
};

#endif
