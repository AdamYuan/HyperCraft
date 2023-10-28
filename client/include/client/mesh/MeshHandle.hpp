#ifndef HYPERCRAFT_CLIENT_MESH_HPP
#define HYPERCRAFT_CLIENT_MESH_HPP

#include <memory>
#include <optional>
#include <vk_mem_alloc.h>

#include "MeshCluster.hpp"
#include "MeshPool.hpp"

namespace hc::client::mesh {

template <typename Vertex, typename Index, typename Info> class MeshHandle {
private:
	std::shared_ptr<MeshPool<Vertex, Index, Info>> m_pool_ptr;
	std::shared_ptr<MeshCluster<Vertex, Index, Info>> m_cluster_ptr;

	VmaVirtualAllocation m_vertex_alloc{VK_NULL_HANDLE}, m_index_alloc{VK_NULL_HANDLE};

	uint64_t m_mesh_id{};
	bool m_finalize{};

	using LocalInsert = typename MeshPool<Vertex, Index, Info>::LocalInsert;
	using LocalErase = typename MeshPool<Vertex, Index, Info>::LocalErase;

	inline std::optional<LocalInsert> construct(const std::shared_ptr<MeshPool<Vertex, Index, Info>> &pool_ptr,
	                                            const std::vector<Vertex> &vertices, const std::vector<Index> &indices,
	                                            const Info &info) {
		m_pool_ptr = pool_ptr;
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
			for (const auto &weak_cluster : pool_ptr->m_clusters) {
				std::shared_ptr<MeshCluster<Vertex, Index, Info>> cluster = weak_cluster.lock();
				if (cluster && cluster->try_alloc(vertices.size(), &m_vertex_alloc, &vertex_offset, indices.size(),
				                                  &m_index_alloc, &index_offset)) {
					m_cluster_ptr = cluster;
					break;
				}
			}
			if (!m_cluster_ptr) {
				for (uint32_t offset = 0; offset < pool_ptr->m_clusters.size(); ++offset) {
					auto &weak_cluster = pool_ptr->m_clusters[offset];
					if (!weak_cluster.expired())
						continue;
					auto cluster = MeshCluster<Vertex, Index, Info>::Create(
					    pool_ptr->GetDevicePtr(), pool_ptr->gen_cluster_id(), offset, //
					    pool_ptr->GetVertexBlockSize(), pool_ptr->GetIndexBlockSize(),
					    pool_ptr->GetMaxMeshesPerCluster());
					weak_cluster = cluster;
					if (cluster->try_alloc(vertices.size(), &m_vertex_alloc, &vertex_offset, indices.size(),
					                       &m_index_alloc, &index_offset))
						m_cluster_ptr = cluster;
					// else, then the vertex and index buffers are too large for a cluster, just ignore it
					break;
				}
			}
		}

		if (!m_cluster_ptr)
			return std::nullopt;

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
		return LocalInsert(m_cluster_ptr->GetClusterID(), m_mesh_id, std::move(vertex_staging),
		                   std::move(index_staging), mesh_info);
	}
	inline std::optional<LocalErase> destruct() {
		if (m_finalize)
			return std::nullopt;
		m_finalize = true;
		return LocalErase(m_cluster_ptr->GetClusterID(), m_mesh_id, m_vertex_alloc, m_index_alloc);
	}

	template <typename, typename, typename> friend class MeshHandleTransaction;

public:
	inline const auto &GetPoolPtr() const { return m_pool_ptr; }
	inline uint64_t GetMeshID() const { return m_mesh_id; }

	inline static std::unique_ptr<MeshHandle> Create(const std::shared_ptr<MeshPool<Vertex, Index, Info>> &pool_ptr,
	                                                 const std::vector<Vertex> &vertices,
	                                                 const std::vector<Index> &indices, const Info &info) {
		auto ret = std::make_unique<MeshHandle>();
		auto opt_local_insert = ret->construct(pool_ptr, vertices, indices, info);
		if (opt_local_insert) {
			pool_ptr->m_local_update_queue.enqueue({{std::move(opt_local_insert.value())}, {}});
			return ret;
		}
		return nullptr;
	}

	inline void SetFinalize() { m_finalize = true; }

	inline ~MeshHandle() {
		auto opt_local_erase = destruct();
		if (opt_local_erase)
			m_pool_ptr->m_local_update_queue.enqueue({{}, {std::move(opt_local_erase.value())}});
	}
};

template <typename Vertex, typename Index, typename Info> class MeshHandleTransaction {
private:
	using LocalTransaction = typename MeshPool<Vertex, Index, Info>::LocalTransaction;
	std::shared_ptr<MeshPool<Vertex, Index, Info>> m_pool_ptr;
	LocalTransaction m_local_transaction;

public:
	inline explicit MeshHandleTransaction(const std::shared_ptr<MeshPool<Vertex, Index, Info>> &pool_ptr)
	    : m_pool_ptr{pool_ptr} {}
	inline std::unique_ptr<MeshHandle<Vertex, Index, Info>>
	Create(const std::vector<Vertex> &vertices, const std::vector<Index> &indices, const Info &info) {
		auto ret = std::make_unique<MeshHandle<Vertex, Index, Info>>();
		auto opt_local_insert = ret->construct(m_pool_ptr, vertices, indices, info);
		if (opt_local_insert) {
			m_local_transaction.inserts.push_back(std::move(opt_local_insert.value()));
			return ret;
		}
		return nullptr;
	}
	inline void Destroy(std::unique_ptr<MeshHandle<Vertex, Index, Info>> handler) {
		if (handler->m_pool_ptr != m_pool_ptr)
			return;
		auto opt_local_erase = handler->destruct();
		if (opt_local_erase)
			m_local_transaction.erases.push_back(std::move(opt_local_erase.value()));
	}
	inline void Submit() {
		if (m_local_transaction.inserts.empty() && m_local_transaction.erases.empty())
			return;
		m_pool_ptr->m_local_update_queue.enqueue(std::move(m_local_transaction));
		m_local_transaction = {};
	}
	inline ~MeshHandleTransaction() { Submit(); }
};

} // namespace hc::client::mesh

#endif
