#ifndef CUBECRAFT3_CLIENT_MESH_RENDERER_HPP
#define CUBECRAFT3_CLIENT_MESH_RENDERER_HPP

#include "MeshCluster.hpp"
#include "MeshInfo.hpp"

#include <atomic>
#include <shared_mutex>
#include <unordered_set>

#include <concurrentqueue.h>

#include <myvk/CommandBuffer.hpp>

template <typename Vertex, typename Index, typename Info> class MeshPool : public myvk::DeviceObjectBase {
public:
	struct LocalUpdate {
		enum class Type { kInsert, kErase };
		Type type;
		uint64_t cluster_id, mesh_id;
		inline explicit LocalUpdate(Type type, uint64_t cluster_id, uint64_t mesh_id)
		    : type{type}, cluster_id{cluster_id}, mesh_id{mesh_id} {}
		inline virtual ~LocalUpdate() = default;
	};
	struct LocalInsert final : public LocalUpdate {
		myvk::Ptr<myvk::Buffer> vertex_staging, index_staging;
		MeshInfo<Info> mesh_info;
		inline LocalInsert(uint64_t cluster_id, uint64_t mesh_id, myvk::Ptr<myvk::Buffer> &&vertex_staging,
		                   myvk::Ptr<myvk::Buffer> &&index_staging, const MeshInfo<Info> &mesh_info)
		    : LocalUpdate(LocalUpdate::Type::kInsert, cluster_id, mesh_id), vertex_staging{std::move(vertex_staging)},
		      index_staging{std::move(index_staging)}, mesh_info{mesh_info} {}
		inline ~LocalInsert() final = default;
	};
	struct LocalErase final : public LocalUpdate {
		VmaVirtualAllocation vertex_alloc, index_alloc;
		inline explicit LocalErase(uint64_t cluster_id, uint64_t mesh_id, VmaVirtualAllocation vertex_alloc,
		                           VmaVirtualAllocation index_alloc)
		    : LocalUpdate(LocalUpdate::Type::kErase, cluster_id, mesh_id), vertex_alloc{vertex_alloc},
		      index_alloc{index_alloc} {}
		inline ~LocalErase() final = default;
	};

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
	inline uint64_t gen_mesh_id() { return m_mesh_id_counter.fetch_add(1, std::memory_order_acq_rel); }
	inline uint64_t gen_cluster_id() { return m_cluster_id_counter.fetch_add(1, std::memory_order_acq_rel); }

	// Local Updates
	moodycamel::ConcurrentQueue<std::unique_ptr<LocalUpdate>> m_local_update_queue;
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

	inline void PostUpdate(std::vector<std::unique_ptr<LocalUpdate>> &&post_updates) {
		auto cluster_map = make_cluster_map();

		for (auto &post_update : post_updates) {
			auto cluster_it = cluster_map.find(post_update->cluster_id);
			if (cluster_it == cluster_map.end())
				continue;

			const std::shared_ptr<MeshCluster<Vertex, Index, Info>> &cluster = cluster_it->second;

			if (post_update->type == LocalUpdate::Type::kErase) {
				auto *p_erase = static_cast<LocalErase *>(post_update.get());
				cluster->free_alloc(p_erase->vertex_alloc, p_erase->index_alloc);
			}
			post_update = nullptr;
		}
	}

	inline void CmdLocalUpdate(const myvk::Ptr<myvk::CommandBuffer> &command_buffer,
	                           std::vector<std::shared_ptr<MeshCluster<Vertex, Index, Info>>> *p_prepared_clusters,
	                           std::vector<std::unique_ptr<LocalUpdate>> *p_post_updates,
	                           VkDeviceSize max_transfer_bytes) {
		auto cluster_map = make_cluster_map();

		p_prepared_clusters->clear();
		p_post_updates->clear();

		std::vector<VkCopyBufferInfo2> copies;
		std::vector<VkBufferCopy2> copy_regions;
		std::vector<VkBufferMemoryBarrier2> post_barriers;

		VkDeviceSize transferred_bytes{};
		std::unique_ptr<LocalUpdate> local_update;
		bool transfer = false;
		while (transferred_bytes < max_transfer_bytes && m_local_update_queue.try_dequeue(local_update)) {
			auto cluster_it = cluster_map.find(local_update->cluster_id);
			if (cluster_it == cluster_map.end())
				continue;

			const std::shared_ptr<MeshCluster<Vertex, Index, Info>> &cluster = cluster_it->second;

			if (local_update->type == LocalUpdate::Type::kInsert) {
				transfer = true;

				auto *p_insert = static_cast<LocalInsert *>(local_update.get());

				auto erased_it = m_erased_meshes.find(p_insert->mesh_id);
				// If the mesh is already erased, just ignore it
				if (erased_it == m_erased_meshes.end()) {
					cluster->local_insert_mesh(p_insert->mesh_id, p_insert->mesh_info, p_insert->vertex_staging,
					                           p_insert->index_staging, &copies, &copy_regions, &post_barriers);
					transferred_bytes += p_insert->vertex_staging->GetSize() + p_insert->index_staging->GetSize();
				} else
					m_erased_meshes.erase(erased_it);
			} else { // Erase
				auto *p_erase = static_cast<LocalErase *>(local_update.get());
				if (cluster->try_local_erase_mesh(p_erase->mesh_id) == false)
					m_erased_meshes.insert(p_erase->mesh_id);
			}
			++cluster->m_cluster_local_version;
			p_post_updates->push_back(std::move(local_update));
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

#endif
