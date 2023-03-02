#ifndef CUBECRAFT3_CLIENT_MESH_RENDERER_HPP
#define CUBECRAFT3_CLIENT_MESH_RENDERER_HPP

#include <client/MeshCluster.hpp>
#include <client/MeshHandle.hpp>
#include <shared_mutex>

// Descriptor Set Placement
// #0: Mesh Info
// #1: DrawCmd Count Buffer [pass1, pass2, ...]
// #2: DrawCmd Buffer for pass1
// #3: DrawCmd Buffer for pass2
// #4: ...

template <typename Vertex, typename Index, typename Info> class MeshRendererBase {
private:
	myvk::Ptr<myvk::Device> m_device;
	std::shared_ptr<myvk::DescriptorSetLayout> m_descriptor_set_layout;

	std::vector<std::weak_ptr<MeshCluster<Vertex, Index, Info>>> m_cluster_vector;
	mutable std::shared_mutex m_cluster_vector_mutex;

	uint32_t m_vertices_block_size{}, m_indices_block_size{}, m_log_2_max_meshes;

public:
	explicit MeshRendererBase(const myvk::Ptr<myvk::Device> &device, uint32_t vertices_block_size,
	                          uint32_t indices_block_size, uint32_t log_2_max_meshes)
	    : m_device{device}, m_vertices_block_size{vertices_block_size}, m_indices_block_size{indices_block_size},
	      m_log_2_max_meshes{log_2_max_meshes} {
		std::vector<VkDescriptorSetLayoutBinding> bindings = {
		    // mesh info buffer
		    {0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1,
		     VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT},
		};
		m_descriptor_set_layout = myvk::DescriptorSetLayout::Create(device, bindings);
	}

	inline const std::shared_ptr<myvk::DescriptorSetLayout> &GetDescriptorSetLayout() const {
		return m_descriptor_set_layout;
	}
	inline uint32_t GetMaxMeshes() const { return 1u << m_log_2_max_meshes; }

	struct PreparedCluster {
		std::shared_ptr<MeshCluster<Vertex, Index, Info>> m_cluster_ptr;
		uint32_t m_mesh_count;
	};

public:
	inline std::vector<PreparedCluster> CmdUpdateMesh(const myvk::Ptr<myvk::CommandBuffer> &command_buffer) const {
		std::vector<PreparedCluster> ret = {};
		std::shared_lock read_lock{m_cluster_vector_mutex};
		for (auto &i : m_cluster_vector) {
			std::shared_ptr<MeshCluster<Vertex, Index, Info>> cluster = i.lock();
			if (cluster) {
				uint32_t mesh_count = cluster->UpdateMesh(command_buffer);
				if (mesh_count)
					ret.push_back({std::move(cluster), mesh_count});
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
			auto new_cluster = MeshCluster<Vertex, Index, Info>::Create(
			    m_device, m_descriptor_set_layout, m_vertices_block_size, m_indices_block_size, m_log_2_max_meshes);
			m_cluster_vector.push_back(new_cluster);
			return MeshHandle<Vertex, Index, Info>::Create(new_cluster, vertices, indices, info);
		}
	}
};

#endif
