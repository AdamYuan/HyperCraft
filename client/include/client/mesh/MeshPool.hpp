#ifndef HYPERCRAFT_CLIENT_MESH_RENDERER_HPP
#define HYPERCRAFT_CLIENT_MESH_RENDERER_HPP

#include "MeshCluster.hpp"
#include "MeshInfo.hpp"

#include <atomic>
#include <shared_mutex>
#include <unordered_set>
#include <variant>

#include <concurrentqueue.h>

#include <myvk/CommandBuffer.hpp>

namespace hc::client::mesh {

template <typename Vertex, typename Index, typename Info> class MeshPool : public myvk::DeviceObjectBase {
public:
	struct LocalInsert {
		uint64_t cluster_id, mesh_id;
		myvk::Ptr<myvk::Buffer> vertex_staging, index_staging;
		MeshInfo<Info> mesh_info;
		inline LocalInsert(uint64_t cluster_id, uint64_t mesh_id, myvk::Ptr<myvk::Buffer> &&vertex_staging,
		                   myvk::Ptr<myvk::Buffer> &&index_staging, const MeshInfo<Info> &mesh_info)
		    : cluster_id{cluster_id}, mesh_id{mesh_id}, vertex_staging{std::move(vertex_staging)},
		      index_staging{std::move(index_staging)}, mesh_info{mesh_info} {}
	};
	struct LocalErase {
		uint64_t cluster_id, mesh_id;
		VmaVirtualAllocation vertex_alloc, index_alloc;
		inline LocalErase(uint64_t cluster_id, uint64_t mesh_id, VmaVirtualAllocation vertex_alloc,
		                  VmaVirtualAllocation index_alloc)
		    : cluster_id{cluster_id}, mesh_id{mesh_id}, vertex_alloc{vertex_alloc}, index_alloc{index_alloc} {}
	};
	struct LocalTransaction {
		std::vector<LocalInsert> inserts;
		std::vector<LocalErase> erases;
	};

	using LocalUpdateEntry = LocalTransaction;
	using PostUpdateEntry = std::variant<LocalInsert, LocalErase>;

private:
	myvk::Ptr<myvk::Device> m_device_ptr;

	// Clusters
	std::vector<std::weak_ptr<MeshCluster<Vertex, Index, Info>>> m_clusters;
	mutable std::shared_mutex m_clusters_mutex;
	myvk::Ptr<myvk::Buffer> m_mesh_info_buffer;

	// Properties
	VkDeviceSize m_vertex_block_size{}, m_index_block_size{};
	uint32_t m_max_clusters{}, m_max_meshes_per_cluster{};

	// ID Counters
	std::atomic_uint64_t m_mesh_id_counter{1}, m_cluster_id_counter{1};
	inline uint64_t gen_mesh_id() { return m_mesh_id_counter.fetch_add(1, std::memory_order_relaxed); }
	inline uint64_t gen_cluster_id() { return m_cluster_id_counter.fetch_add(1, std::memory_order_relaxed); }

	// Local Updates
	moodycamel::ConcurrentQueue<LocalUpdateEntry> m_local_update_queue;
	std::unordered_set<uint64_t> m_erased_meshes;

	inline std::unordered_map<uint64_t, std::shared_ptr<MeshCluster<Vertex, Index, Info>>> make_cluster_map() const {
		std::unordered_map<uint64_t, std::shared_ptr<MeshCluster<Vertex, Index, Info>>> cluster_map;
		{
			std::shared_lock read_lock{m_clusters_mutex};
			for (auto &i : m_clusters) {
				auto cluster = i.lock();
				if (cluster) {
					uint64_t cluster_id = cluster->GetClusterID();
					cluster_map.insert({cluster_id, std::move(cluster)});
				}
			}
		}
		return cluster_map;
	}

	template <typename, typename, typename> friend class MeshHandle;
	template <typename, typename, typename> friend class MeshHandleTransaction;
	template <typename, typename, typename> friend class MeshCluster;

public:
	explicit MeshPool(const myvk::Ptr<myvk::Device> &device, VkDeviceSize vertex_block_size,
	                  VkDeviceSize index_block_size, uint32_t max_clusters, uint32_t max_meshes_per_cluster)
	    : m_device_ptr{device}, m_vertex_block_size{vertex_block_size}, m_index_block_size{index_block_size},
	      m_max_clusters{max_clusters}, m_max_meshes_per_cluster{max_meshes_per_cluster} {
		m_clusters.resize(max_clusters);
		m_mesh_info_buffer =
		    myvk::Buffer::Create(device, max_clusters * max_meshes_per_cluster * sizeof(MeshInfo<Info>),
		                         VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT,
		                         VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
	}

	inline const myvk::Ptr<myvk::Device> &GetDevicePtr() const final { return m_device_ptr; }
	inline const auto &GetMeshInfoBuffer() const { return m_mesh_info_buffer; }
	inline uint32_t GetMaxClusters() const { return m_max_clusters; }
	inline uint32_t GetMaxMeshesPerCluster() const { return m_max_meshes_per_cluster; }
	inline VkDeviceSize GetVertexBlockSize() const { return m_vertex_block_size; }
	inline VkDeviceSize GetIndexBlockSize() const { return m_index_block_size; }

	inline void PostUpdate(std::vector<PostUpdateEntry> &&post_updates) {
		if (post_updates.empty())
			return;

		auto cluster_map = make_cluster_map();

		for (auto &post_update : post_updates) {
			std::visit(
			    [&cluster_map](const auto &post_erase) {
				    if constexpr (std::is_same_v<std::decay_t<decltype(post_erase)>, LocalErase>) {
					    auto cluster_it = cluster_map.find(post_erase.cluster_id);
					    if (cluster_it == cluster_map.end())
						    return;
					    const std::shared_ptr<MeshCluster<Vertex, Index, Info>> &cluster = cluster_it->second;
					    cluster->free_alloc(post_erase.vertex_alloc, post_erase.index_alloc);
				    }
			    },
			    post_update);
		}
	}

	inline void CmdLocalUpdate(const myvk::Ptr<myvk::CommandBuffer> &command_buffer,
	                           std::vector<std::shared_ptr<MeshCluster<Vertex, Index, Info>>> *p_prepared_clusters,
	                           std::vector<PostUpdateEntry> *p_post_updates, VkDeviceSize max_transfer_bytes) {
		auto cluster_map = make_cluster_map();

		p_prepared_clusters->clear();
		// p_post_updates->clear();

		std::vector<VkCopyBufferInfo2> copies;
		std::vector<VkBufferCopy2> copy_regions;
		std::vector<VkBufferMemoryBarrier2> post_barriers;

		VkDeviceSize transferred_bytes{};
		LocalUpdateEntry local_update;
		bool transfer = false;
		while (transferred_bytes < max_transfer_bytes && m_local_update_queue.try_dequeue(local_update)) {
			for (auto &erase : local_update.erases) {
				auto cluster_it = cluster_map.find(erase.cluster_id);
				if (cluster_it != cluster_map.end()) {
					const std::shared_ptr<MeshCluster<Vertex, Index, Info>> &cluster = cluster_it->second;
					if (cluster->try_local_erase_mesh(erase.mesh_id) == false)
						m_erased_meshes.insert(erase.mesh_id); // if not erased, mark it as erased
					++cluster->m_cluster_local_version;
				}
				p_post_updates->push_back(std::move(erase));
			}
			for (auto &insert : local_update.inserts) {
				auto cluster_it = cluster_map.find(insert.cluster_id);
				if (cluster_it != cluster_map.end()) {
					const std::shared_ptr<MeshCluster<Vertex, Index, Info>> &cluster = cluster_it->second;
					transfer = true;

					auto erased_it = m_erased_meshes.find(insert.mesh_id);
					// If the mesh is marked as erased, just ignore it
					if (erased_it == m_erased_meshes.end()) {
						cluster->local_insert_mesh(insert.mesh_id, insert.mesh_info, insert.vertex_staging,
						                           insert.index_staging, &copies, &copy_regions, &post_barriers);
						transferred_bytes += insert.vertex_staging->GetSize() + insert.index_staging->GetSize();
					} else
						m_erased_meshes.erase(erased_it);
					++cluster->m_cluster_local_version;
				}
				p_post_updates->push_back(std::move(insert));
			}
		}

		// Copy
		if (transfer) {
			command_buffer->CmdPipelineBarrier(VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, {},
			                                   {}, {});
			for (uint32_t i = 0; i < copies.size(); ++i) {
				copies[i].regionCount = 1;
				copies[i].pRegions = copy_regions.data() + i;
				vkCmdCopyBuffer2(command_buffer->GetHandle(), copies.data() + i);
			}

			VkDependencyInfo dep_info = {VK_STRUCTURE_TYPE_DEPENDENCY_INFO};
			dep_info.bufferMemoryBarrierCount = post_barriers.size();
			dep_info.pBufferMemoryBarriers = post_barriers.data();
			vkCmdPipelineBarrier2(command_buffer->GetHandle(), &dep_info);
		}

		for (auto &cluster_it : cluster_map) {
			auto &cluster = cluster_it.second;
			if (cluster->GetLocalMeshCount() > 0) {
				p_prepared_clusters->push_back(std::move(cluster));
			}
		}
		std::sort(p_prepared_clusters->begin(), p_prepared_clusters->end(),
		          [](const auto &l, const auto &r) { return l->GetClusterOffset() < r->GetClusterOffset(); });
	}
};

template <typename Vertex, typename Index, typename Info> class MeshInfoBuffer final : public myvk::BufferBase {
private:
	myvk::Ptr<MeshPool<Vertex, Index, Info>> m_pool_ptr;
	VmaAllocation m_allocation{VK_NULL_HANDLE};

	struct ClusterState {
		uint64_t id{}, version{};
		MeshInfo<Info> *p_data{};
	};
	std::vector<ClusterState> m_cluster_states;

public:
	inline static myvk::Ptr<MeshInfoBuffer> Create(myvk::Ptr<MeshPool<Vertex, Index, Info>> pool_ptr,
	                                               VkBufferUsageFlags usages = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT) {
		auto ret = std::make_shared<MeshInfoBuffer>();
		ret->m_pool_ptr = pool_ptr;
		ret->m_size = pool_ptr->GetMaxClusters() * pool_ptr->GetMaxMeshesPerCluster() * sizeof(MeshInfo<Info>);

		VkBufferCreateInfo create_info = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
		create_info.size = ret->m_size;
		create_info.usage = usages;
		create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		VmaAllocationCreateInfo alloc_create_info = {};
		alloc_create_info.usage = VMA_MEMORY_USAGE_AUTO;
		alloc_create_info.flags =
		    VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;

		VmaAllocationInfo alloc_info;
		if (vmaCreateBuffer(pool_ptr->GetDevicePtr()->GetAllocatorHandle(), &create_info, &alloc_create_info,
		                    &ret->m_buffer, &ret->m_allocation, &alloc_info) != VK_SUCCESS)
			return nullptr;
		auto p_data = static_cast<MeshInfo<Info> *>(alloc_info.pMappedData);

		ret->m_cluster_states.resize(pool_ptr->GetMaxClusters());
		for (uint32_t i = 0; i < pool_ptr->GetMaxClusters(); ++i) {
			auto &state = ret->m_cluster_states[i];
			state.p_data = p_data + i * pool_ptr->GetMaxMeshesPerCluster();
		}
		return ret;
	}

	inline void Update(const std::vector<std::shared_ptr<MeshCluster<Vertex, Index, Info>>> &prepared_clusters) {
		for (const auto &cluster : prepared_clusters) {
			auto &state = m_cluster_states[cluster->GetClusterOffset()];
			if (cluster->GetClusterID() == state.id && cluster->GetClusterLocalVersion() <= state.version)
				continue;
			MeshInfo<Info> *p_data = state.p_data;
			for (const auto &it : cluster->m_local_mesh_info_map)
				*(p_data)++ = it.second;
			state.id = cluster->GetClusterID();
			state.version = cluster->GetClusterLocalVersion();
			// printf("Update: id = %d, version = %d, offset = %d\n", state.id, state.version,
			//        cluster->GetClusterOffset());
		}
	}

	inline const myvk::Ptr<myvk::Device> &GetDevicePtr() const final { return m_pool_ptr->GetDevicePtr(); }

	inline ~MeshInfoBuffer() final {
		if (m_buffer)
			vmaDestroyBuffer(m_pool_ptr->GetDevicePtr()->GetAllocatorHandle(), m_buffer, m_allocation);
	}
};

} // namespace hc::client::mesh

#endif
