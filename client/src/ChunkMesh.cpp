#include <client/ChunkMesh.hpp>

#include <client/WorldRenderer.hpp>

#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>

#include <spdlog/spdlog.h>

bool ChunkMesh::Register() {
	std::shared_ptr<Chunk> chunk;
	std::shared_ptr<WorldRenderer> world_renderer;
	{
		chunk = m_chunk_weak_ptr.lock();
		if (!chunk)
			return false;

		std::shared_ptr<World> world = chunk->LockWorld();
		if (!world)
			return false;
		world_renderer = world->LockWorldRenderer();
		if (!world_renderer)
			return false;
	}
	world_renderer->upload_chunk_mesh(chunk, shared_from_this());
	return true;
}

bool ChunkMesh::Update(const ChunkMesh::UpdateInfo &update_info) {
	std::shared_ptr<WorldRenderer> world_renderer;
	{ // Access transfer queue
		std::shared_ptr<Chunk> chunk = m_chunk_weak_ptr.lock();
		if (!chunk)
			return false;

		// Initialize base position
		m_base_pos = (glm::i32vec3)chunk->GetPosition() * (int32_t)Chunk::kSize;

		std::shared_ptr<World> world = chunk->LockWorld();
		if (!world)
			return false;
		world_renderer = world->LockWorldRenderer();
		if (!world_renderer)
			return false;
	}
	// if empty, set as nullptr and return
	if (update_info.indices.empty()) {
		m_updated_buffer.store(nullptr, std::memory_order_release);
		m_updated.store(true, std::memory_order_release);
		return true;
	}

	// update AABB
	m_aabb = (fAABB)((i32AABB)update_info.aabb + m_base_pos);

	// upload buffer
	const std::shared_ptr<myvk::Queue> &transfer_queue = world_renderer->GetTransferQueue();

	std::shared_ptr<myvk::Buffer> m_vertices_staging = myvk::Buffer::CreateStaging(
	    transfer_queue->GetDevicePtr(), update_info.vertices.begin(), update_info.vertices.end());
	std::shared_ptr<myvk::Buffer> m_indices_staging = myvk::Buffer::CreateStaging(
	    transfer_queue->GetDevicePtr(), update_info.indices.begin(), update_info.indices.end());

	std::shared_ptr<myvk::CommandPool> command_pool = myvk::CommandPool::Create(transfer_queue);
	std::shared_ptr<myvk::CommandBuffer> command_buffer = myvk::CommandBuffer::Create(command_pool);
	{
		std::shared_ptr<myvk::Buffer> buffer = myvk::Buffer::Create(
		    transfer_queue->GetDevicePtr(), m_vertices_staging->GetSize() + m_indices_staging->GetSize(),
		    VMA_MEMORY_USAGE_GPU_ONLY,
		    VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);

		command_buffer->Begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
		command_buffer->CmdCopy(m_vertices_staging, buffer, {{0, 0, m_vertices_staging->GetSize()}});
		command_buffer->CmdCopy(m_indices_staging, buffer,
		                        {{0, m_vertices_staging->GetSize(), m_indices_staging->GetSize()}});
		command_buffer->End();

		std::shared_ptr<myvk::Fence> fence = myvk::Fence::Create(transfer_queue->GetDevicePtr());
		command_buffer->Submit(fence);
		fence->Wait();

		m_updated_buffer.store(buffer, std::memory_order_release);
		m_updated.store(true, std::memory_order_release);
	}

	return true;
}
bool ChunkMesh::CmdDraw(const std::shared_ptr<myvk::CommandBuffer> &command_buffer,
                        const std::shared_ptr<myvk::PipelineLayout> &pipeline_layout, const Frustum &frustum,
                        uint32_t frame) {
	if (m_updated.exchange(false, std::memory_order_acq_rel))
		m_buffer = m_updated_buffer.load(std::memory_order_acquire);

	m_frame_buffers[frame] = m_buffer;
	if (!m_buffer) {
		if (std::all_of(m_frame_buffers, m_frame_buffers + kFrameCount,
		                [](const std::shared_ptr<myvk::Buffer> &x) -> bool { return x == nullptr; })) {
			return true;
		}
	} else if (!frustum.Cull(m_aabb)) {
		uint32_t face_count = m_buffer->GetSize() / (4 * sizeof(Vertex) + 6 * sizeof(uint16_t));
		command_buffer->CmdPushConstants(pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT, 0, 3 * sizeof(int32_t),
		                                 glm::value_ptr(m_base_pos));
		command_buffer->CmdBindVertexBuffer(m_buffer, 0);
		command_buffer->CmdBindIndexBuffer(m_buffer, face_count * (4 * sizeof(Vertex)), VK_INDEX_TYPE_UINT16);

		command_buffer->CmdDrawIndexed(face_count * 6, 1, 0, 0, 0);
	}
	return false;
}
