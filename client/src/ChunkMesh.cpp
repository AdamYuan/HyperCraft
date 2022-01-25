#include <client/ChunkMesh.hpp>

#include <client/WorldRenderer.hpp>

std::shared_ptr<ChunkMesh> ChunkMesh::Allocate(const std::shared_ptr<Chunk> &chunk_ptr) {
	std::scoped_lock lock{chunk_ptr->m_mesh_mutex};

	std::shared_ptr<ChunkMesh> ret = chunk_ptr->LockMesh();
	if (ret)
		return ret;
	std::shared_ptr<WorldRenderer> world_renderer;
	{
		std::shared_ptr<World> world = chunk_ptr->LockWorld();
		if (!world)
			return nullptr;
		world_renderer = world->LockWorldRenderer();
		if (!world_renderer)
			return nullptr;
	}
	ret = std::make_shared<ChunkMesh>();
	ret->m_chunk_weak_ptr = chunk_ptr;
	world_renderer->upload_chunk_mesh(chunk_ptr->GetPosition(), ret);
	chunk_ptr->m_mesh_weak_ptr = ret;
	return ret;
}

void ChunkMesh::Update(const ChunkMesh::UpdateInfo &update_info) {
	std::shared_ptr<WorldRenderer> world_renderer;
	{ // Access transfer queue
		std::shared_ptr<Chunk> chunk = m_chunk_weak_ptr.lock();
		if (!chunk)
			return;
		std::shared_ptr<World> world = chunk->LockWorld();
		if (!world)
			return;
		world_renderer = world->LockWorldRenderer();
		if (!world_renderer)
			return;
	}
	if (update_info.indices.empty()) {
		m_buffer.store(nullptr, std::memory_order_release);
		return;
	}
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

		m_buffer.store(buffer, std::memory_order_release);
	}

	spdlog::info("Vertex ({} byte) and Index buffer ({} byte) uploaded", m_vertices_staging->GetSize(),
	             m_indices_staging->GetSize());
}
bool ChunkMesh::CmdDraw(const std::shared_ptr<myvk::CommandBuffer> &command_buffer,
                        const std::shared_ptr<myvk::PipelineLayout> &pipeline_layout, const ChunkPos3 &chunk_pos,
                        uint32_t frame) {
	m_frame_buffers[frame] = m_buffer.load(std::memory_order_acquire);
	const std::shared_ptr<myvk::Buffer> &current_buffer = m_frame_buffers[frame];

	if (!current_buffer) {
		if (std::all_of(m_frame_buffers, m_frame_buffers + kFrameCount,
		                [](const std::shared_ptr<myvk::Buffer> &x) -> bool { return x == nullptr; })) {
			return true;
		}
	} else {
		int32_t pos[] = {(int32_t)chunk_pos.x * (int32_t)Chunk::kSize, (int32_t)chunk_pos.y * (int32_t)Chunk::kSize,
		                 (int32_t)chunk_pos.z * (int32_t)Chunk::kSize};
		uint32_t face_count = current_buffer->GetSize() / (4 * sizeof(Vertex) + 6 * sizeof(uint16_t));
		command_buffer->CmdPushConstants(pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT, 0, 3 * sizeof(int32_t), pos);
		command_buffer->CmdBindVertexBuffer(current_buffer, 0);
		command_buffer->CmdBindIndexBuffer(current_buffer, face_count * (4 * sizeof(Vertex)), VK_INDEX_TYPE_UINT16);

		command_buffer->CmdDrawIndexed(face_count * 6, 1, 0, 0, 0);
	}
	return false;
}
