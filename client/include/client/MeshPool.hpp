#ifndef CUBECRAFT3_CLIENT_MESH_RENDERER_HPP
#define CUBECRAFT3_CLIENT_MESH_RENDERER_HPP

#include <client/MeshCluster.hpp>
#include <client/MeshHandle.hpp>

#include <concurrentqueue.h>

#include <atomic>
#include <shared_mutex>

// Descriptor Set Placement
// #0: Mesh Info
template <typename Vertex, typename Index, typename Info> class MeshPool {
private:
	std::shared_ptr<MeshSynchronizer<Vertex, Index, Info>> m_synchronizer_ptr;

	std::shared_ptr<myvk::DescriptorSetLayout> m_descriptor_set_layout;

	std::vector<std::weak_ptr<MeshCluster<Vertex, Index, Info>>> m_cluster_vector;
	mutable std::shared_mutex m_cluster_vector_mutex;

	uint32_t m_vertices_block_size{}, m_indices_block_size{}, m_log_2_max_meshes_per_cluster{};

	mutable std::vector<myvk::Ptr<myvk::Buffer>> m_transferred;

public:
	explicit MeshPool(const myvk::Ptr<myvk::Device> &device, uint32_t vertices_block_size, uint32_t indices_block_size,
	                  uint32_t log_2_max_meshes_per_cluster)
	    : m_vertices_block_size{vertices_block_size}, m_indices_block_size{indices_block_size},
	      m_log_2_max_meshes_per_cluster{log_2_max_meshes_per_cluster} {
		std::vector<VkDescriptorSetLayoutBinding> bindings = {
		    // mesh info buffer
		    {0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1,
		     VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT},
		};
		m_descriptor_set_layout = myvk::DescriptorSetLayout::Create(device, bindings);
		m_synchronizer_ptr = std::make_shared<MeshSynchronizer<Vertex, Index, Info>>();
	}

	inline const std::shared_ptr<myvk::DescriptorSetLayout> &GetDescriptorSetLayout() const {
		return m_descriptor_set_layout;
	}
	inline uint32_t GetMaxMeshesPerCluster() const { return 1u << m_log_2_max_meshes_per_cluster; }

	inline std::vector<std::shared_ptr<MeshCluster<Vertex, Index, Info>>>
	CmdUpdateMesh(const myvk::Ptr<myvk::CommandBuffer> &command_buffer, VkDeviceSize max_transfer_bytes) const {
		std::unordered_map<uint64_t, std::shared_ptr<MeshCluster<Vertex, Index, Info>>> cluster_map;

		std::shared_lock read_lock{m_cluster_vector_mutex};
		for (auto &i : m_cluster_vector) {
			auto cluster = i.lock();
			if (cluster)
				cluster_map.insert({cluster->GetClusterID(), cluster});
		}

		VkDeviceSize transfer_bytes;
		while (transfer_bytes < max_transfer_bytes) {
			// std::unique_ptr
			auto update_ptr = m_synchronizer_ptr->DequeueMeshUpdate();
			if (update_ptr == nullptr)
				break;

			auto cluster_it = cluster_map.find(update_ptr->cluster_id);
			if (cluster_it == cluster_map.end())
				continue;

			// const std::shared_ptr &
			const auto &cluster = cluster_it->second;

			if (update_ptr->type == MeshSynchronizer<Vertex, Index, Info>::Update::Type::kInsert) {
				auto *p_insert =
				    static_cast<typename MeshSynchronizer<Vertex, Index, Info>::Insert *>(update_ptr.get());

				// TAG UPDATE
				cluster->CmdLocalInsertMesh(command_buffer, *p_insert);
				transfer_bytes += p_insert->vertex_staging->GetSize() + p_insert->index_staging->GetSize();
				m_transferred.push_back(std::move(p_insert->vertex_staging));
				m_transferred.push_back(std::move(p_insert->index_staging));
			} else { // Erase
				auto *p_erase = static_cast<typename MeshSynchronizer<Vertex, Index, Info>::Erase *>(update_ptr.get());
				cluster->LocalEraseMesh(*p_erase);
			}
		}

		std::vector<std::shared_ptr<MeshCluster<Vertex, Index, Info>>> ret;
		ret.reserve(cluster_map.size());

		for (auto &cluster_it : cluster_map) {
			auto &cluster = cluster_it.second;
			if (cluster->GetLocalMeshCount() > 0) {
				if (cluster->PopLocalUpdated()) {
					cluster->CmdUpdateMeshInfo(command_buffer);
				}
				ret.push_back(std::move(cluster));
			}
		}
		return ret;
	}

	inline std::unique_ptr<MeshHandle<Vertex, Index, Info>>
	PushMesh(const std::vector<Vertex> &vertices, const std::vector<Index> &indices, const Info &info) {
		if (vertices.empty() || indices.empty())
			return nullptr;
		std::unique_ptr<MeshHandle<Vertex, Index, Info>> ret{};
		{ // If can find a free cluster, just alloc
			std::shared_lock read_lock{m_cluster_vector_mutex};
			for (auto &i : m_cluster_vector) {
				auto cluster = i.lock();
				if (cluster && (ret = MeshHandle<Vertex, Index, Info>::Create(cluster, vertices, indices, info)))
					return ret;
			}
		}
		{ // If not found,
			std::scoped_lock write_lock{m_cluster_vector_mutex};
			// Avoid push vector multiple times
			for (auto &i : m_cluster_vector) {
				auto cluster = i.lock();
				if (cluster && (ret = MeshHandle<Vertex, Index, Info>::Create(cluster, vertices, indices, info)))
					return ret;
			}
			// Remove all expired clusters
			m_cluster_vector.erase(
			    std::remove_if(m_cluster_vector.begin(), m_cluster_vector.end(),
			                   [&](const std::weak_ptr<MeshCluster<Vertex, Index, Info>> &x) { return x.expired(); }),
			    m_cluster_vector.end());
			// Push a new cluster
			auto new_cluster = MeshCluster<Vertex, Index, Info>::Create(m_synchronizer_ptr, m_descriptor_set_layout,
			                                                            m_vertices_block_size, m_indices_block_size,
			                                                            m_log_2_max_meshes_per_cluster);
			m_cluster_vector.push_back(new_cluster);
			return MeshHandle<Vertex, Index, Info>::Create(new_cluster, vertices, indices, info);
		}
	}
};

#endif
