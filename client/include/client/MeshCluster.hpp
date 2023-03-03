#ifndef CUBECRAFT3_CLIENT_MESH_GROUP_HPP
#define CUBECRAFT3_CLIENT_MESH_GROUP_HPP

#include <atomic>
#include <memory>
#include <shared_mutex>
#include <unordered_map>

#include <vk_mem_alloc.h>

#include <myvk/Buffer.hpp>
#include <myvk/CommandBuffer.hpp>
#include <myvk/Queue.hpp>

#include <client/Config.hpp>

#include <concurrentqueue.h>
#include <spdlog/spdlog.h>

constexpr uint32_t kMaxMeshes = 65536;

inline uint32_t group_x_64(uint32_t x) { return (x >> 6u) + (((x & 0x3fu) > 0u) ? 1u : 0u); }

template <typename Vertex, typename Index, typename Info> class MeshCluster;

// Mesh information
template <typename Info> struct MeshInfo {
	uint32_t m_index_count, m_first_index, m_vertex_offset;
	Info m_info;
};

// Mesh synchronizer
template <typename Vertex, typename Index, typename Info> class MeshSynchronizer {
public:
	struct Update {
		enum class Type { kInsert, kErase };
		Type type;
		uint64_t cluster_id, mesh_id;

		inline explicit Update(Type type, uint64_t cluster_id, uint64_t mesh_id)
		    : type{type}, cluster_id{cluster_id}, mesh_id{mesh_id} {}
		inline virtual ~Update() = default;
	};
	struct Insert final : public Update {
		myvk::Ptr<myvk::Buffer> vertex_staging, index_staging;
		MeshInfo<Info> mesh_info;
		inline Insert(uint64_t cluster_id, uint64_t mesh_id, myvk::Ptr<myvk::Buffer> &&vertex_staging,
		              myvk::Ptr<myvk::Buffer> &&index_staging, const MeshInfo<Info> &mesh_info)
		    : Update(Update::Type::kInsert, cluster_id, mesh_id), vertex_staging{std::move(vertex_staging)},
		      index_staging{std::move(index_staging)}, mesh_info{mesh_info} {}
		inline ~Insert() final = default;
	};
	struct Erase final : public Update {
		inline explicit Erase(uint64_t cluster_id, uint64_t mesh_id)
		    : Update(Update::Type::kErase, cluster_id, mesh_id) {}
		inline ~Erase() final = default;
	};

private:
	moodycamel::ConcurrentQueue<std::unique_ptr<Update>> m_mesh_update_queue;
	std::atomic_uint64_t m_mesh_id_counter{}, m_cluster_id_counter{};

public:
	inline uint64_t NewClusterID() { return m_cluster_id_counter.fetch_add(1, std::memory_order_acq_rel); }
	inline uint64_t EnqueueMeshInsert(uint64_t cluster_id, //
	                                  myvk::Ptr<myvk::Buffer> &&vertex_staging, VkDeviceSize vertex_offset,
	                                  myvk::Ptr<myvk::Buffer> &&index_staging, VkDeviceSize index_offset,
	                                  const Info &info) {
		auto mesh_id = m_mesh_id_counter.fetch_add(1, std::memory_order_acq_rel);

		MeshInfo<Info> mesh_info = {
		    uint32_t(index_staging->GetSize() / sizeof(Index)),
		    uint32_t(index_offset / sizeof(Index)),
		    uint32_t(vertex_offset / sizeof(Vertex)),
		    info,
		};
		auto mesh_insert = std::make_unique<Insert>(cluster_id, mesh_id, std::move(vertex_staging),
		                                            std::move(index_staging), mesh_info);
		m_mesh_update_queue.enqueue(std::move(mesh_insert));

		return mesh_id;
	}
	inline void EnqueueMeshErase(uint64_t cluster_id, uint64_t mesh_id) {
		auto mesh_erase = std::make_unique<Erase>(cluster_id, mesh_id);
		m_mesh_update_queue.enqueue(std::move(mesh_erase));
	}
	inline std::unique_ptr<Update> DequeueMeshUpdate() {
		std::unique_ptr<Update> ret{nullptr};
		m_mesh_update_queue.try_dequeue(ret);
		return ret;
	}
};

// Used for zero-overhead rendering (with GPU culling)
template <typename Vertex, typename Index, typename Info> class MeshCluster {
	static_assert(sizeof(Index) == 4 || sizeof(Index) == 2 || sizeof(Index) == 1, "sizeof Index must be 1, 2 or 4");

private:
	uint64_t m_cluster_id = -1;
	uint32_t m_max_meshes{};
	std::shared_ptr<MeshSynchronizer<Vertex, Index, Info>> m_synchronizer_ptr;

	myvk::Ptr<myvk::DescriptorSet> m_descriptor_set;
	myvk::Ptr<myvk::Buffer> m_mesh_info_buffer_staging, m_mesh_info_buffer;
	myvk::Ptr<myvk::Buffer> m_vertex_buffer, m_index_buffer;

	// Local Info
	std::unordered_map<uint64_t, MeshInfo<Info>> m_local_mesh_info_map;
	bool m_local_inserted{}, m_local_updated{};

	uint32_t m_allocation_count{};
	VmaVirtualBlock m_vertex_virtual_block{}, m_index_virtual_block{};
	std::mutex m_allocation_mutex{};

	// returns first_index
	inline uint64_t enqueue_mesh_insert(const std::vector<Vertex> &vertex, VkDeviceSize vertex_offset,
	                                    const std::vector<Index> &index, VkDeviceSize index_offset, const Info &info) {
		std::shared_ptr<myvk::Buffer> vertex_staging =
		    myvk::Buffer::CreateStaging(m_descriptor_set->GetDevicePtr(), vertex.begin(), vertex.end());
		std::shared_ptr<myvk::Buffer> index_staging =
		    myvk::Buffer::CreateStaging(m_descriptor_set->GetDevicePtr(), index.begin(), index.end());

		return m_synchronizer_ptr->EnqueueMeshInsert(m_cluster_id, std::move(vertex_staging), vertex_offset,
		                                             std::move(index_staging), index_offset, info);
	}
	inline void enqueue_mesh_erase(uint64_t mesh_id) { m_synchronizer_ptr->EnqueueMeshErase(m_cluster_id, mesh_id); }

	template <typename, typename, typename> friend class MeshHandle;
	template <typename, typename, typename> friend class MeshEraser;

public:
	inline static std::shared_ptr<MeshCluster>
	Create(const std::shared_ptr<MeshSynchronizer<Vertex, Index, Info>> &synchronizer_ptr,
	       const myvk::Ptr<myvk::DescriptorSetLayout> &descriptor_set_layout, VkDeviceSize vertex_buffer_size,
	       VkDeviceSize index_buffer_size, uint32_t log_2_max_meshes) {
		const auto &device = descriptor_set_layout->GetDevicePtr();

		auto ret = std::make_shared<MeshCluster>();
		ret->m_synchronizer_ptr = synchronizer_ptr;
		ret->m_cluster_id = synchronizer_ptr->NewClusterID();
		ret->m_vertex_buffer =
		    myvk::Buffer::Create(device, vertex_buffer_size, VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT,
		                         VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
		ret->m_index_buffer =
		    myvk::Buffer::Create(device, index_buffer_size, VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT,
		                         VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);

		ret->m_max_meshes = 1u << log_2_max_meshes;

		ret->m_mesh_info_buffer = myvk::Buffer::Create(
		    device, kMaxMeshes * sizeof(MeshInfo<Info>), VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT,
		    VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);

		ret->m_mesh_info_buffer_staging =
		    myvk::Buffer::Create(device, kMaxMeshes * sizeof(MeshInfo<Info>),
		                         VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT |
		                             VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT,
		                         VK_BUFFER_USAGE_TRANSFER_SRC_BIT);

		VmaVirtualBlockCreateInfo virtual_block_create_info = {};
		virtual_block_create_info.size = vertex_buffer_size;
		if (vmaCreateVirtualBlock(&virtual_block_create_info, &ret->m_vertex_virtual_block) != VK_SUCCESS)
			return nullptr;

		virtual_block_create_info.size = index_buffer_size;
		if (vmaCreateVirtualBlock(&virtual_block_create_info, &ret->m_index_virtual_block) != VK_SUCCESS) {
			vmaDestroyVirtualBlock(ret->m_vertex_virtual_block);
			return nullptr;
		}

		ret->m_descriptor_set = myvk::DescriptorSet::Create(
		    myvk::DescriptorPool::Create(device, 1, {{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1}}), descriptor_set_layout);
		ret->m_descriptor_set->UpdateStorageBuffer(ret->m_mesh_info_buffer, 0);

		return ret;
	}
	~MeshCluster() {
		vmaDestroyVirtualBlock(m_vertex_virtual_block);
		vmaDestroyVirtualBlock(m_index_virtual_block);
	}

	inline uint64_t GetClusterID() const { return m_cluster_id; }

	inline uint32_t GetLocalMeshCount() const { return m_local_mesh_info_map.size(); }
	inline uint32_t GetMaxMeshes() const { return m_max_meshes; }
	inline const auto &GetMeshInfoDescriptorSet() const { return m_descriptor_set; }
	inline const auto &GetVertexBuffer() const { return m_vertex_buffer; }
	inline const auto &GetIndexBuffer() const { return m_index_buffer; }

	inline void CmdLocalInsertMesh(const myvk::Ptr<myvk::CommandBuffer> &command_buffer,
	                               const typename MeshSynchronizer<Vertex, Index, Info>::Insert &insert) {
		// TODO: Output Barriers
		assert(m_cluster_id == insert.cluster_id);
		m_local_mesh_info_map.insert({insert.mesh_id, insert.mesh_info});
		command_buffer->CmdCopy(
		    insert.vertex_staging, m_vertex_buffer,
		    {{0, (VkDeviceSize)insert.mesh_info.m_vertex_offset * sizeof(Vertex), insert.vertex_staging->GetSize()}});
		command_buffer->CmdCopy(
		    insert.index_staging, m_index_buffer,
		    {{0, (VkDeviceSize)insert.mesh_info.m_first_index * sizeof(Index), insert.index_staging->GetSize()}});
		m_local_updated = m_local_inserted = true;
	}

	inline void LocalEraseMesh(const typename MeshSynchronizer<Vertex, Index, Info>::Erase &erase) {
		assert(m_cluster_id == erase.cluster_id);
		m_local_mesh_info_map.erase(erase.mesh_id);
		m_local_updated = true;
	}

	inline bool PopLocalUpdated() {
		bool ret = m_local_updated;
		m_local_updated = false;
		return ret;
	}
	inline bool PopLocalInserted() {
		bool ret = m_local_inserted;
		m_local_inserted = false;
		return ret;
	}

	inline void CmdUpdateMeshInfo(const myvk::Ptr<myvk::CommandBuffer> &command_buffer) const {
		auto mesh_info_data = (MeshInfo<Info> *)m_mesh_info_buffer_staging->GetMappedData();
		for (const auto &it : m_local_mesh_info_map)
			*(mesh_info_data++) = it.second;
		command_buffer->CmdCopy(m_mesh_info_buffer_staging, m_mesh_info_buffer,
		                        {{0, 0, GetLocalMeshCount() * sizeof(MeshInfo<Info>)}});
		// TODO: Barriers
	}

	inline static constexpr VkIndexType kIndexType =
	    sizeof(Index) == 4 ? VK_INDEX_TYPE_UINT32
	                       : (sizeof(Index) == 2 ? VK_INDEX_TYPE_UINT16 : VK_INDEX_TYPE_UINT8_EXT);
};

#endif
