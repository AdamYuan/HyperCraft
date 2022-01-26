#include <client/WorldRenderer.hpp>

#include <myvk/ShaderModule.hpp>
#include <spdlog/spdlog.h>

void WorldRenderer::upload_chunk_mesh(const std::shared_ptr<Chunk> &chunk_ptr,
                                      const std::shared_ptr<ChunkMesh> &chunk_mesh_ptr) {
	std::scoped_lock lock{m_chunk_meshes_mutex};

	m_chunk_meshes[chunk_ptr->GetPosition()] = chunk_mesh_ptr;
	chunk_ptr->m_mesh_weak_ptr = chunk_mesh_ptr;

	spdlog::info("chunk mesh count: {}", m_chunk_meshes.size());
}

void WorldRenderer::create_pipeline(const std::shared_ptr<myvk::RenderPass> &render_pass, uint32_t subpass) {
	const std::shared_ptr<myvk::Device> &device = m_transfer_queue->GetDevicePtr();
	m_pipeline_layout = myvk::PipelineLayout::Create(
	    device, {m_texture_ptr->GetDescriptorSetLayout(), m_camera_ptr->GetDescriptorSetLayout()},
	    {{VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(int32_t) * 3}});

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
	pipeline_state.m_vertex_input_state.Enable({{0, sizeof(ChunkMesh::Vertex), VK_VERTEX_INPUT_RATE_VERTEX}},
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
	    {m_texture_ptr->GetDescriptorSet(), m_camera_ptr->GetFrameDescriptorSet(current_frame)}, m_pipeline);

	VkRect2D scissor = {};
	scissor.extent = extent;
	command_buffer->CmdSetScissor({scissor});
	VkViewport viewport = {};
	viewport.width = (float)extent.width;
	viewport.height = (float)extent.height;
	viewport.maxDepth = 1.0f;
	command_buffer->CmdSetViewport({viewport});

	{
		std::scoped_lock lock{m_chunk_meshes_mutex};

		for (auto i = m_chunk_meshes.begin(); i != m_chunk_meshes.end();) {
			if (i->second->CmdDraw(command_buffer, m_pipeline_layout, m_camera_ptr->GetFrustum(), current_frame))
				i = m_chunk_meshes.erase(i);
			else
				++i;
		}
	}
}
