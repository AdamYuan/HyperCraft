#ifndef CUBECRAFT3_CLIENT_MESH_GROUP_HPP
#define CUBECRAFT3_CLIENT_MESH_GROUP_HPP

#include <atomic>
#include <memory>
#include <shared_mutex>
#include <unordered_map>

#include <vk_mem_alloc.h>

#include <myvk/Buffer.hpp>

#include "MeshInfo.hpp"

#include <spdlog/include/spdlog/spdlog.h>

// Used for zero-overhead rendering (with GPU culling)
template <typename Vertex, typename Index, typename Info> class MeshCluster {
	static_assert(sizeof(Index) == 4 || sizeof(Index) == 2 || sizeof(Index) == 1, "sizeof Index must be 1, 2 or 4");

private:
	uint32_t m_cluster_offset{}, m_max_meshes{};
	uint64_t m_cluster_id{}, m_cluster_local_version{1};

	// Buffers
	myvk::Ptr<myvk::Buffer> m_vertex_buffer, m_index_buffer;

	// Local Info
	std::unordered_map<uint64_t, MeshInfo<Info>> m_local_mesh_info_map;

	// Allocations
	uint32_t m_allocation_count{};
	VmaVirtualBlock m_vertex_virtual_block{}, m_index_virtual_block{};
	std::mutex m_allocation_mutex{};

	inline bool try_alloc(std::size_t vertices, VmaVirtualAllocation *p_vertex_alloc, VkDeviceSize *p_vertex_offset,
	                      std::size_t indices, VmaVirtualAllocation *p_index_alloc, VkDeviceSize *p_index_offset) {
		VmaVirtualAllocationCreateInfo vertex_alloc_info = {}, index_alloc = {};
		vertex_alloc_info.flags = VMA_VIRTUAL_ALLOCATION_CREATE_STRATEGY_MIN_MEMORY_BIT;
		vertex_alloc_info.alignment = sizeof(Vertex);
		static_assert((sizeof(Vertex) & (sizeof(Vertex) - 1)) == 0, "sizeof(Vertex) is required to be power of 2");
		vertex_alloc_info.size = vertices * sizeof(Vertex);

		index_alloc.flags = VMA_VIRTUAL_ALLOCATION_CREATE_STRATEGY_MIN_MEMORY_BIT;
		index_alloc.alignment = sizeof(Index);
		static_assert((sizeof(Index) & (sizeof(Index) - 1)) == 0, "sizeof(Index) is required to be power of 2");
		index_alloc.size = indices * sizeof(Index);

		{
			std::scoped_lock allocation_lock{m_allocation_mutex};
			if (m_allocation_count >= m_max_meshes)
				return false;
			if (vmaVirtualAllocate(m_vertex_virtual_block, &vertex_alloc_info, p_vertex_alloc, p_vertex_offset) !=
			    VK_SUCCESS) {
				return false;
			}
			if (vmaVirtualAllocate(m_index_virtual_block, &index_alloc, p_index_alloc, p_index_offset) != VK_SUCCESS) {
				// Free vertex block if index block allocation failed
				vmaVirtualFree(m_vertex_virtual_block, *p_vertex_alloc);
				return false;
			}
			++m_allocation_count;
		}
		return true;
	}

	inline void free_alloc(VmaVirtualAllocation vertex_alloc, VmaVirtualAllocation index_alloc) {
		std::scoped_lock allocation_lock{m_allocation_mutex};
		if (vertex_alloc)
			vmaVirtualFree(m_vertex_virtual_block, vertex_alloc);
		if (index_alloc)
			vmaVirtualFree(m_index_virtual_block, index_alloc);
		--m_allocation_count;
	}

	inline void local_insert_mesh(uint64_t mesh_id, const MeshInfo<Info> &mesh_info,
	                              const myvk::Ptr<myvk::Buffer> &vertex_staging,
	                              const myvk::Ptr<myvk::Buffer> &index_staging,
	                              std::vector<VkCopyBufferInfo2> *p_copies, std::vector<VkBufferCopy2> *p_copy_regions,
	                              std::vector<VkBufferMemoryBarrier2> *p_post_barriers) {
		m_local_mesh_info_map.insert({mesh_id, mesh_info});
		{ // Vertex
			p_copies->push_back({VK_STRUCTURE_TYPE_COPY_BUFFER_INFO_2});
			auto &copy = p_copies->back();
			copy.srcBuffer = vertex_staging->GetHandle();
			copy.dstBuffer = m_vertex_buffer->GetHandle();

			p_copy_regions->push_back({VK_STRUCTURE_TYPE_BUFFER_COPY_2});
			auto &copy_region = p_copy_regions->back();
			copy_region.size = vertex_staging->GetSize();
			copy_region.dstOffset = (VkDeviceSize)mesh_info.m_vertex_offset * sizeof(Vertex);

			p_post_barriers->push_back({VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2});
			auto &post_barrier = p_post_barriers->back();
			post_barrier.size = copy_region.size;
			post_barrier.offset = copy_region.dstOffset;
			post_barrier.buffer = m_vertex_buffer->GetHandle();
			post_barrier.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
			post_barrier.srcStageMask = VK_PIPELINE_STAGE_2_COPY_BIT;
			post_barrier.dstAccessMask = VK_ACCESS_2_VERTEX_ATTRIBUTE_READ_BIT;
			post_barrier.dstStageMask = VK_PIPELINE_STAGE_2_VERTEX_ATTRIBUTE_INPUT_BIT;
		}
		{ // Index
			p_copies->push_back({VK_STRUCTURE_TYPE_COPY_BUFFER_INFO_2});
			auto &copy = p_copies->back();
			copy.srcBuffer = index_staging->GetHandle();
			copy.dstBuffer = m_index_buffer->GetHandle();

			p_copy_regions->push_back({VK_STRUCTURE_TYPE_BUFFER_COPY_2});
			auto &copy_region = p_copy_regions->back();
			copy_region.size = index_staging->GetSize();
			copy_region.dstOffset = (VkDeviceSize)mesh_info.m_first_index * sizeof(Index);

			p_post_barriers->push_back({VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2});
			auto &post_barrier = p_post_barriers->back();
			post_barrier.size = copy_region.size;
			post_barrier.offset = copy_region.dstOffset;
			post_barrier.buffer = m_index_buffer->GetHandle();
			post_barrier.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
			post_barrier.srcStageMask = VK_PIPELINE_STAGE_2_COPY_BIT;
			post_barrier.dstAccessMask = VK_ACCESS_2_INDEX_READ_BIT;
			post_barrier.dstStageMask = VK_PIPELINE_STAGE_2_INDEX_INPUT_BIT;
		}
	}

	inline bool try_local_erase_mesh(uint64_t mesh_id) { return m_local_mesh_info_map.erase(mesh_id); }

	template <typename, typename, typename> friend class MeshHandle;
	template <typename, typename, typename> friend class MeshInfoBuffer;
	template <typename, typename, typename> friend class MeshPool;

public:
	inline MeshCluster(const myvk::Ptr<myvk::Device> &device, uint64_t cluster_id, uint32_t cluster_offset,
	                   VkDeviceSize vertex_buffer_size, VkDeviceSize index_buffer_size, uint32_t max_meshes)
	    : m_cluster_id{cluster_id}, m_cluster_offset{cluster_offset}, m_max_meshes{max_meshes} {
		/* m_mesh_info_staging =
		    myvk::Buffer::Create(device, max_meshes * sizeof(MeshInfo<Info>),
		                         VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT |
		                             VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT,
		                         VK_BUFFER_USAGE_TRANSFER_SRC_BIT); */
		m_vertex_buffer = myvk::Buffer::Create(device, vertex_buffer_size, VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT,
		                                       VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
		m_index_buffer = myvk::Buffer::Create(device, index_buffer_size, VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT,
		                                      VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
		VmaVirtualBlockCreateInfo virtual_block_create_info = {};
		virtual_block_create_info.size = vertex_buffer_size;
		vmaCreateVirtualBlock(&virtual_block_create_info, &m_vertex_virtual_block);
		virtual_block_create_info.size = index_buffer_size;
		vmaCreateVirtualBlock(&virtual_block_create_info, &m_index_virtual_block);
	}
	~MeshCluster() {
		vmaDestroyVirtualBlock(m_vertex_virtual_block);
		vmaDestroyVirtualBlock(m_index_virtual_block);
	}

	inline static auto Create(const myvk::Ptr<myvk::Device> &device, uint64_t cluster_id, uint32_t cluster_offset,
	                          VkDeviceSize vertex_buffer_size, VkDeviceSize index_buffer_size, uint32_t max_meshes) {
		return std::make_shared<MeshCluster>(device, cluster_id, cluster_offset, vertex_buffer_size, index_buffer_size,
		                                     max_meshes);
	}

	inline uint64_t GetClusterID() const { return m_cluster_id; }
	inline uint64_t GetClusterLocalVersion() const { return m_cluster_local_version; }
	inline uint32_t GetClusterOffset() const { return m_cluster_offset; }
	inline uint32_t GetLocalMeshCount() const { return m_local_mesh_info_map.size(); }
	inline uint32_t GetMaxMeshes() const { return m_max_meshes; }

	inline const auto &GetVertexBuffer() const { return m_vertex_buffer; }
	inline const auto &GetIndexBuffer() const { return m_index_buffer; }

	inline static constexpr VkIndexType kIndexType =
	    sizeof(Index) == 4 ? VK_INDEX_TYPE_UINT32
	                       : (sizeof(Index) == 2 ? VK_INDEX_TYPE_UINT16 : VK_INDEX_TYPE_UINT8_EXT);
};

#endif
