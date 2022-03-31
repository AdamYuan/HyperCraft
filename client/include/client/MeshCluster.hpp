#ifndef CUBECRAFT3_CLIENT_MESH_GROUP_HPP
#define CUBECRAFT3_CLIENT_MESH_GROUP_HPP

#include <atomic>
#include <memory>
#include <shared_mutex>

#include <atomic_shared_ptr.hpp>
#include <vk_mem_alloc.h>

#include <myvk/Buffer.hpp>
#include <myvk/CommandBuffer.hpp>
#include <myvk/Queue.hpp>

#include <client/Config.hpp>

#include <spdlog/spdlog.h>

inline uint32_t group_x_64(uint32_t x) { return (x >> 6u) + (((x & 0x3fu) > 0u) ? 1u : 0u); }

// Descriptor Set Placement
// #0: Mesh Info
// #1: DrawCmd Count Buffer [pass1, pass2, ...]
// #2: DrawCmd Buffer for pass1
// #3: DrawCmd Buffer for pass2
// #4: ...

// Used for zero-overhead rendering (with GPU culling)
template <typename Vertex, typename Index, typename Info, uint32_t kPassCount = 1> class MeshCluster {
	static_assert(sizeof(Index) == 4 || sizeof(Index) == 2 || sizeof(Index) == 1, "sizeof Index must be 1, 2 or 4");
	static_assert(kPassCount > 0, "pass count must be greater than 0");

private:
	std::shared_ptr<myvk::Queue> m_transfer_queue;
	std::shared_ptr<myvk::Buffer> m_vertex_buffer, m_index_buffer;

	VmaVirtualBlock m_vertices_virtual_block{}, m_indices_virtual_block{};
	std::mutex m_vertices_virtual_block_mutex, m_indices_virtual_block_mutex;

	// Mesh information
	struct MeshInfo {
		uint32_t m_index_count, m_first_index, m_vertex_offset;
		Info m_info;
	};
	std::shared_mutex m_mesh_info_vector_mutex;
	std::vector<MeshInfo> m_mesh_info_vector;
	folly::atomic_shared_ptr<myvk::Buffer> m_mesh_info_buffer;
	std::shared_ptr<myvk::Buffer> m_frame_mesh_info_buffers[kFrameCount];

	// Indirect command buffers
	// draw command count for vkDrawIndexedIndirectCount
	std::shared_ptr<myvk::Buffer> m_frame_draw_count_buffers[kFrameCount];
	// indirect draw commands for vkCmdDrawIndexedIndirectCount (separated with an offset for each pass)
	std::shared_ptr<myvk::Buffer> m_frame_draw_command_buffers[kFrameCount][kPassCount];

	// Descriptors
	std::shared_ptr<myvk::DescriptorPool> m_descriptor_pool;
	std::shared_ptr<myvk::DescriptorSet> m_frame_descriptor_sets[kFrameCount];

	inline void upload_mesh_content(const std::vector<Vertex> &vertices, VkDeviceSize vertices_offset,
	                                const std::vector<Index> &indices, VkDeviceSize indices_offset) {
		std::shared_ptr<myvk::Buffer> vertices_staging =
		    myvk::Buffer::CreateStaging(m_transfer_queue->GetDevicePtr(), vertices.begin(), vertices.end());
		std::shared_ptr<myvk::Buffer> indices_staging =
		    myvk::Buffer::CreateStaging(m_transfer_queue->GetDevicePtr(), indices.begin(), indices.end());

		std::shared_ptr<myvk::CommandPool> command_pool = myvk::CommandPool::Create(m_transfer_queue);
		std::shared_ptr<myvk::CommandBuffer> command_buffer = myvk::CommandBuffer::Create(command_pool);
		{
			command_buffer->Begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
			command_buffer->CmdCopy(vertices_staging, m_vertex_buffer,
			                        {{0, vertices_offset, vertices_staging->GetSize()}});
			command_buffer->CmdCopy(indices_staging, m_index_buffer, {{0, indices_offset, indices_staging->GetSize()}});

			// TODO: Queue ownership transfer

			command_buffer->End();

			std::shared_ptr<myvk::Fence> fence = myvk::Fence::Create(m_transfer_queue->GetDevicePtr());
			command_buffer->Submit(fence);
			fence->Wait();
		}
	}

	inline void upload_mesh_info_vector() {
		// executed inside mesh_info_mutex
		std::shared_ptr<myvk::Buffer> staging;
		{
			std::shared_lock read_lock{m_mesh_info_vector_mutex};
			if (m_mesh_info_vector.empty()) {
				read_lock.unlock();
				m_mesh_info_buffer.store(nullptr, std::memory_order_release);
				return;
			}
			staging = myvk::Buffer::CreateStaging(m_transfer_queue->GetDevicePtr(), m_mesh_info_vector.begin(),
			                                      m_mesh_info_vector.end());
		}
		std::shared_ptr<myvk::CommandPool> command_pool = myvk::CommandPool::Create(m_transfer_queue);
		std::shared_ptr<myvk::CommandBuffer> command_buffer = myvk::CommandBuffer::Create(command_pool);
		{
			std::shared_ptr<myvk::Buffer> mesh_info_buffer =
			    myvk::Buffer::Create(m_transfer_queue->GetDevicePtr(), staging->GetSize(), 0,
			                         VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			                         VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE);
			command_buffer->Begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
			command_buffer->CmdCopy(staging, mesh_info_buffer, {{0, 0, staging->GetSize()}});
			// TODO: Queue ownership transfer (Maybe)

			command_buffer->End();

			std::shared_ptr<myvk::Fence> fence = myvk::Fence::Create(m_transfer_queue->GetDevicePtr());
			command_buffer->Submit(fence);
			fence->Wait();

			m_mesh_info_buffer.store(mesh_info_buffer, std::memory_order_release);
		}
	}

	// returns first_index
	inline uint32_t insert_mesh(const std::vector<Vertex> &vertices, VkDeviceSize vertices_offset,
	                            const std::vector<Index> &indices, VkDeviceSize indices_offset, const Info &info) {
		upload_mesh_content(vertices, vertices_offset, indices, indices_offset);

		MeshInfo mesh_info = {(uint32_t)indices.size(), (uint32_t)(indices_offset / sizeof(Index)),
		                      (uint32_t)(vertices_offset / sizeof(Vertex)), info};
		{
			std::scoped_lock write_lock{m_mesh_info_vector_mutex};
			m_mesh_info_vector.push_back(mesh_info);
		}
		upload_mesh_info_vector();
		return mesh_info.m_first_index;
	}
	inline void erase_mesh_without_upload(uint32_t first_index) {
		std::scoped_lock write_lock{m_mesh_info_vector_mutex};
		auto it = std::find_if(m_mesh_info_vector.begin(), m_mesh_info_vector.end(),
		                       [first_index](const MeshInfo &info) { return info.m_first_index == first_index; });
		if (it != m_mesh_info_vector.end()) {
			*it = std::move(m_mesh_info_vector.back());
			m_mesh_info_vector.pop_back();
		}
	}
	inline void erase_mesh(uint32_t first_index) {
		erase_mesh_without_upload(first_index);
		upload_mesh_info_vector();
	}

	template <typename, typename, typename, uint32_t> friend class MeshHandle;
	template <typename, typename, typename, uint32_t> friend class MeshEraser;

public:
	inline static std::shared_ptr<MeshCluster>
	Create(const std::shared_ptr<myvk::Queue> &transfer_queue,
	       const std::shared_ptr<myvk::DescriptorSetLayout> &descriptor_set_layout, VkDeviceSize vertex_buffer_size,
	       VkDeviceSize index_buffer_size) {
		auto ret = std::make_shared<MeshCluster>();
		ret->m_transfer_queue = transfer_queue;
		ret->m_vertex_buffer = myvk::Buffer::Create(
		    transfer_queue->GetDevicePtr(), vertex_buffer_size, VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT,
		    VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
		ret->m_index_buffer = myvk::Buffer::Create(transfer_queue->GetDevicePtr(), index_buffer_size,
		                                           VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT,
		                                           VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);

		VmaVirtualBlockCreateInfo virtual_block_create_info = {};
		virtual_block_create_info.size = vertex_buffer_size;
		if (vmaCreateVirtualBlock(&virtual_block_create_info, &ret->m_vertices_virtual_block) != VK_SUCCESS)
			return nullptr;

		virtual_block_create_info.size = index_buffer_size;
		if (vmaCreateVirtualBlock(&virtual_block_create_info, &ret->m_indices_virtual_block) != VK_SUCCESS)
			return nullptr;

		ret->m_descriptor_pool =
		    myvk::DescriptorPool::Create(transfer_queue->GetDevicePtr(), kFrameCount,
		                                 {{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, (2 + kPassCount) * kFrameCount}});
		for (uint32_t i = 0; i < kFrameCount; ++i) {
			ret->m_frame_draw_count_buffers[i] = myvk::Buffer::Create(
			    transfer_queue->GetDevicePtr(), sizeof(uint32_t) * kPassCount,
			    VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT,
			    VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT, VMA_MEMORY_USAGE_AUTO);
			ret->m_frame_descriptor_sets[i] =
			    myvk::DescriptorSet::Create(ret->m_descriptor_pool, descriptor_set_layout);
			ret->m_frame_descriptor_sets[i]->UpdateStorageBuffer(ret->m_frame_draw_count_buffers[i], 1);
		}
		return ret;
	}
	~MeshCluster() {
		for (auto &i : m_frame_draw_count_buffers)
			i->Unmap();
		vmaDestroyVirtualBlock(m_vertices_virtual_block);
		vmaDestroyVirtualBlock(m_indices_virtual_block);
	}

	inline const std::shared_ptr<myvk::DescriptorSet> &GetFrameDescriptorSet(uint32_t current_frame) const {
		return m_frame_descriptor_sets[current_frame];
	}
	// Return mesh count
	inline uint32_t PrepareFrame(uint32_t current_frame) {
		// fetch mesh buffer
		m_frame_mesh_info_buffers[current_frame] = m_mesh_info_buffer.load(std::memory_order_acquire);
		const std::shared_ptr<myvk::Buffer> &mesh_info_buffer = m_frame_mesh_info_buffers[current_frame];
		if (mesh_info_buffer == nullptr)
			return 0;

		uint32_t mesh_count = mesh_info_buffer->GetSize() / sizeof(MeshInfo);

		const std::shared_ptr<myvk::DescriptorSet> &descriptor_set = m_frame_descriptor_sets[current_frame];
		descriptor_set->UpdateStorageBuffer(mesh_info_buffer, 0);

		// reset counters
		{
			auto count_ptrs = (uint32_t *)m_frame_draw_count_buffers[current_frame]->GetMappedData();
			std::fill(count_ptrs, count_ptrs + kPassCount, 0u);
		}

		// check draw_command buffer size
		VkDeviceSize desired_drawcmd_buffer_size =
		    (2u << glm::findMSB(mesh_count)) * sizeof(VkDrawIndexedIndirectCommand);
		for (uint32_t p = 0; p < kPassCount; ++p) {
			std::shared_ptr<myvk::Buffer> &draw_command_buffer = m_frame_draw_command_buffers[current_frame][p];
			if ((draw_command_buffer ? draw_command_buffer->GetSize() : 0) < desired_drawcmd_buffer_size) {
				draw_command_buffer =
				    myvk::Buffer::Create(m_transfer_queue->GetDevicePtr(), desired_drawcmd_buffer_size, 0,
				                         VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT,
				                         VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE);
				descriptor_set->UpdateStorageBuffer(draw_command_buffer, 2 + p);
			}
		}

		return mesh_count;
	}
	inline void CmdDispatch(const std::shared_ptr<myvk::CommandBuffer> &command_buffer, uint32_t mesh_count) {
		command_buffer->CmdDispatch(group_x_64(mesh_count), 1, 1);
	}
	inline void CmdBarrier(const std::shared_ptr<myvk::CommandBuffer> &command_buffer, uint32_t current_frame) {
		const std::shared_ptr<myvk::Buffer> &count_buffer = m_frame_draw_count_buffers[current_frame];
		/*const std::shared_ptr<myvk::Buffer> &draw_command_buffer = m_frame_draw_command_buffers[current_frame];
		command_buffer->CmdPipelineBarrier(
		    VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT, {},
		    {
		        count_buffer->GetMemoryBarrier(VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_INDIRECT_COMMAND_READ_BIT),
		        draw_command_buffer->GetMemoryBarrier(VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_INDIRECT_COMMAND_READ_BIT),
		    },
		    {});*/

		std::vector<VkBufferMemoryBarrier> buffer_memory_barriers = {
		    count_buffer->GetMemoryBarrier(VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_INDIRECT_COMMAND_READ_BIT)};
		for (uint32_t p = 0; p < kPassCount; ++p) {
			buffer_memory_barriers.push_back(m_frame_draw_command_buffers[current_frame][p]->GetMemoryBarrier(
			    VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_INDIRECT_COMMAND_READ_BIT));
		}
		command_buffer->CmdPipelineBarrier(VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT,
		                                   {}, buffer_memory_barriers, {});
	}
	inline void CmdDrawIndirect(const std::shared_ptr<myvk::CommandBuffer> &command_buffer, uint32_t current_frame,
	                            uint32_t pass_id, uint32_t mesh_count) {
		const std::shared_ptr<myvk::Buffer> &count_buffer = m_frame_draw_count_buffers[current_frame];
		const std::shared_ptr<myvk::Buffer> &draw_command_buffer = m_frame_draw_command_buffers[current_frame][pass_id];
		constexpr VkIndexType kIndexType = sizeof(Index) == 4
		                                       ? VK_INDEX_TYPE_UINT32
		                                       : (sizeof(Index) == 2 ? VK_INDEX_TYPE_UINT16 : VK_INDEX_TYPE_UINT8_EXT);
		command_buffer->CmdBindVertexBuffer(m_vertex_buffer, 0);
		command_buffer->CmdBindIndexBuffer(m_index_buffer, 0, kIndexType);
		command_buffer->CmdDrawIndexedIndirectCount(draw_command_buffer, 0, count_buffer,
		                                            pass_id * sizeof(uint32_t), // count buffer offset
		                                            mesh_count);
	}
};

#endif
