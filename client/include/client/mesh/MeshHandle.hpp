#ifndef CUBECRAFT3_CLIENT_MESH_HPP
#define CUBECRAFT3_CLIENT_MESH_HPP

#include <memory>
#include <vk_mem_alloc.h>

#include "MeshCluster.hpp"
#include "MeshPool.hpp"

template <typename Vertex, typename Index, typename Info> class MeshHandle {
private:
	std::shared_ptr<MeshPool<Vertex, Index, Info>> m_pool_ptr;
	std::shared_ptr<MeshCluster<Vertex, Index, Info>> m_cluster_ptr;

	VmaVirtualAllocation m_vertex_alloc{VK_NULL_HANDLE}, m_index_alloc{VK_NULL_HANDLE};

	uint64_t m_mesh_id{};
	bool m_finalize{};

public:
	inline const auto &GetPoolPtr() const { return m_pool_ptr; }
	inline uint64_t GetMeshID() const { return m_mesh_id; }

	inline MeshHandle(const std::shared_ptr<MeshPool<Vertex, Index, Info>> &pool_ptr,
	                  const std::vector<Vertex> &vertices, const std::vector<Index> &indices, const Info &info)
	    : m_pool_ptr{pool_ptr} {
		VkDeviceSize vertex_offset, index_offset;
		{
			std::shared_lock read_lock{pool_ptr->m_clusters_mutex};
			for (const auto &weak_cluster : pool_ptr->m_clusters) {
				std::shared_ptr<MeshCluster<Vertex, Index, Info>> cluster = weak_cluster.lock();
				if (cluster && cluster->try_alloc(vertices.size(), &m_vertex_alloc, &vertex_offset, indices.size(),
				                                  &m_index_alloc, &index_offset)) {
					m_cluster_ptr = cluster;
					break;
				}
			}
		}
		if (!m_cluster_ptr) {
			std::scoped_lock write_lock{pool_ptr->m_clusters_mutex};
			for (uint32_t offset = 0; offset < pool_ptr->m_clusters.size(); ++offset) {
				auto &weak_cluster = pool_ptr->m_clusters[offset];
				if (!weak_cluster.expired())
					continue;
				auto cluster = MeshCluster<Vertex, Index, Info>::Create(
				    pool_ptr->GetDevicePtr(), pool_ptr->gen_cluster_id(), offset, //
				    pool_ptr->GetVertexBlockSize(), pool_ptr->GetIndexBlockSize(), pool_ptr->GetMaxMeshesPerCluster());
				weak_cluster = cluster;
				if (cluster->try_alloc(vertices.size(), &m_vertex_alloc, &vertex_offset, indices.size(), &m_index_alloc,
				                       &index_offset))
					m_cluster_ptr = cluster;
				// else, then the vertex and index buffers are too large for a cluster, just ignore it
				return;
			}
		}

		// Enqueue local mesh insert
		std::shared_ptr<myvk::Buffer> vertex_staging =
		    myvk::Buffer::CreateStaging(pool_ptr->GetDevicePtr(), vertices.begin(), vertices.end());
		std::shared_ptr<myvk::Buffer> index_staging =
		    myvk::Buffer::CreateStaging(pool_ptr->GetDevicePtr(), indices.begin(), indices.end());
		m_mesh_id = pool_ptr->gen_mesh_id();
		MeshInfo<Info> mesh_info = {
		    uint32_t(indices.size()),
		    uint32_t(index_offset / sizeof(Index)),
		    uint32_t(vertex_offset / sizeof(Vertex)),
		    info,
		};
		using LocalInsert = typename MeshPool<Vertex, Index, Info>::LocalInsert;
		pool_ptr->m_local_update_queue.enqueue(std::make_unique<LocalInsert>(
		    m_cluster_ptr->GetClusterID(), m_mesh_id, std::move(vertex_staging), std::move(index_staging), mesh_info));
	}

	inline static std::unique_ptr<MeshHandle> Create(const std::shared_ptr<MeshPool<Vertex, Index, Info>> &pool_ptr,
	                                                 const std::vector<Vertex> &vertices,
	                                                 const std::vector<Index> &indices, const Info &info) {
		auto ret = std::make_unique<MeshHandle>(pool_ptr, vertices, indices, info);
		if (ret->m_cluster_ptr)
			return std::move(ret);
		return nullptr;
	}

	inline void SetFinalize() { m_finalize = true; }

	inline ~MeshHandle() {
		if (m_finalize)
			return;
		// Enqueue local mesh erase
		using LocalErase = typename MeshPool<Vertex, Index, Info>::LocalErase;
		m_pool_ptr->m_local_update_queue.enqueue(
		    std::make_unique<LocalErase>(m_cluster_ptr->GetClusterID(), m_mesh_id, m_vertex_alloc, m_index_alloc));
	}
};

#endif
