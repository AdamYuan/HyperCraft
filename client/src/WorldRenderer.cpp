#include <client/WorldRenderer.hpp>
#include <myvk/ShaderModule.hpp>
#include <spdlog/spdlog.h>

void WorldRenderer::UploadMesh(const std::shared_ptr<Chunk> &chunk) {
	std::vector<Chunk::Vertex> vertices;
	std::vector<uint16_t> indices;

	if (!chunk->GetMesh().Pop(&vertices, &indices))
		return;

	{
		std::unique_lock draw_cmd_lock{m_draw_cmd_mutex};
		auto it = m_draw_commands.find(chunk->GetPosition());
		push_draw_cmd(vertices, indices,
		              it == m_draw_commands.end() ? &(m_draw_commands[chunk->GetPosition()]) : &it->second);
	}

	spdlog::info("Chunk ({}, {}, {}) Updated mesh", chunk->GetPosition().x, chunk->GetPosition().y,
	             chunk->GetPosition().z);
	spdlog::info("DrawCmd Size = {}", m_draw_commands.size());
}

void WorldRenderer::push_draw_cmd(const std::vector<Chunk::Vertex> &vertices, const std::vector<uint16_t> &indices,
                                  WorldRenderer::DrawCmd *draw_cmd) const {
	if (indices.empty()) {
		draw_cmd->m_vertex_buffer = nullptr;
		draw_cmd->m_index_buffer = nullptr;

		// TODO: remove empty item

		return;
	}
	std::shared_ptr<myvk::Buffer> m_vertices_staging =
	    myvk::Buffer::CreateStaging(m_transfer_queue->GetDevicePtr(), vertices.begin(), vertices.end());
	std::shared_ptr<myvk::Buffer> m_indices_staging =
	    myvk::Buffer::CreateStaging(m_transfer_queue->GetDevicePtr(), indices.begin(), indices.end());

	draw_cmd->m_vertex_buffer =
	    myvk::Buffer::Create(m_transfer_queue->GetDevicePtr(), m_vertices_staging->GetSize(), VMA_MEMORY_USAGE_GPU_ONLY,
	                         VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
	draw_cmd->m_index_buffer =
	    myvk::Buffer::Create(m_transfer_queue->GetDevicePtr(), m_indices_staging->GetSize(), VMA_MEMORY_USAGE_GPU_ONLY,
	                         VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);

	std::shared_ptr<myvk::CommandPool> command_pool = myvk::CommandPool::Create(m_transfer_queue);
	std::shared_ptr<myvk::CommandBuffer> command_buffer = myvk::CommandBuffer::Create(command_pool);

	command_buffer->Begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
	command_buffer->CmdCopy(m_vertices_staging, draw_cmd->m_vertex_buffer, {{0, 0, m_vertices_staging->GetSize()}});
	command_buffer->CmdCopy(m_indices_staging, draw_cmd->m_index_buffer, {{0, 0, m_indices_staging->GetSize()}});
	command_buffer->End();

	std::shared_ptr<myvk::Fence> fence = myvk::Fence::Create(m_transfer_queue->GetDevicePtr());
	command_buffer->Submit(fence);
	fence->Wait();

	spdlog::info("Vertex ({} byte) and Index buffer ({} byte) uploaded", m_vertices_staging->GetSize(),
	             m_indices_staging->GetSize());
}

void WorldRenderer::create_pipeline(const std::shared_ptr<myvk::RenderPass> &render_pass, uint32_t subpass) {
	const std::shared_ptr<myvk::Device> &device = m_transfer_queue->GetDevicePtr();
	m_pipeline_layout =
	    myvk::PipelineLayout::Create(device, {m_texture->GetDescriptorSetLayout(), m_camera->GetDescriptorSetLayout()},
	                                 {{VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(uint32_t) * 4}});

	constexpr uint32_t kWorldVertSpv[] = {
#include <client/shader/world.vert.u32>
	};
	constexpr uint32_t kWorldFragSpv[] = {
#include <client/shader/world.frag.u32>
	};

	std::shared_ptr<myvk::ShaderModule> vert_shader_module, frag_shader_module;
	vert_shader_module = myvk::ShaderModule::Create(device, kWorldVertSpv, sizeof(kWorldVertSpv));
	frag_shader_module = myvk::ShaderModule::Create(device, kWorldFragSpv, sizeof(kWorldFragSpv));

	std::vector<VkPipelineShaderStageCreateInfo> shader_stages = {
	    vert_shader_module->GetPipelineShaderStageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT),
	    frag_shader_module->GetPipelineShaderStageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT)};

	myvk::GraphicsPipelineState pipeline_state = {};
	pipeline_state.m_vertex_input_state.Enable({{0, sizeof(Chunk::Vertex), VK_VERTEX_INPUT_RATE_VERTEX}},
	                                           {{0, 0, VK_FORMAT_R32G32_UINT, 0}});
	pipeline_state.m_input_assembly_state.Enable(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
	pipeline_state.m_viewport_state.Enable(1, 1);
	pipeline_state.m_rasterization_state.Initialize(VK_POLYGON_MODE_FILL, VK_FRONT_FACE_COUNTER_CLOCKWISE,
	                                                VK_CULL_MODE_BACK_BIT);
	pipeline_state.m_depth_stencil_state.Enable();
	pipeline_state.m_multisample_state.Enable(VK_SAMPLE_COUNT_1_BIT);
	pipeline_state.m_color_blend_state.Enable(1, VK_FALSE);
	pipeline_state.m_dynamic_state.Enable({VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR});

	m_pipeline = myvk::GraphicsPipeline::Create(m_pipeline_layout, render_pass, shader_stages, pipeline_state, subpass);
}
void WorldRenderer::CmdDrawPipeline(const std::shared_ptr<myvk::CommandBuffer> &command_buffer,
                                    const VkExtent2D &extent, uint32_t current_frame) const {
	command_buffer->CmdBindPipeline(m_pipeline);
	command_buffer->CmdBindDescriptorSets(
	    {m_texture->GetDescriptorSet(), m_camera->GetFrameDescriptorSet(current_frame)}, m_pipeline);

	VkRect2D scissor = {};
	scissor.extent = extent;
	command_buffer->CmdSetScissor({scissor});
	VkViewport viewport = {};
	viewport.width = (float)extent.width;
	viewport.height = (float)extent.height;
	viewport.maxDepth = 1.0f;
	command_buffer->CmdSetViewport({viewport});

	{
		std::shared_lock draw_cmd_lock{m_draw_cmd_mutex};

		for (auto &i : m_draw_commands) {
			DrawCmd &cmd = i.second;
			cmd.m_frame_vertices[current_frame] = cmd.m_vertex_buffer;
			cmd.m_frame_indices[current_frame] = cmd.m_index_buffer;

			if (!cmd.m_vertex_buffer) {
				continue;
			}

			command_buffer->CmdBindVertexBuffer(cmd.m_vertex_buffer, 0);
			command_buffer->CmdBindIndexBuffer(cmd.m_index_buffer, 0, VK_INDEX_TYPE_UINT16);

			command_buffer->CmdDrawIndexed(cmd.m_index_buffer->GetSize() / sizeof(uint16_t), 1, 0, 0, 0);
		}
	}
}
