#include <client/ChunkRenderer.hpp>
#include <spdlog/spdlog.h>

void ChunkRenderer::create_culling_pipeline(const std::shared_ptr<myvk::Device> &device) {
	m_culling_pipeline_layout = myvk::PipelineLayout::Create(
	    device,
	    {GetDescriptorSetLayout(), m_camera_ptr->GetDescriptorSetLayout(), m_depth_ptr->GetDescriptorSetLayout()}, {});
	constexpr uint32_t kChunkCullingCompSpv[] = {
#include <client/shader/chunk_culling.comp.u32>
	};
	m_culling_pipeline = myvk::ComputePipeline::Create(
	    m_culling_pipeline_layout,
	    myvk::ShaderModule::Create(device, kChunkCullingCompSpv, sizeof(kChunkCullingCompSpv)));
}

void ChunkRenderer::create_main_pipeline(const std::shared_ptr<myvk::RenderPass> &render_pass, uint32_t subpass) {
	const std::shared_ptr<myvk::Device> &device = render_pass->GetDevicePtr();
	m_main_pipeline_layout = myvk::PipelineLayout::Create(
	    device,
	    {m_texture_ptr->GetDescriptorSetLayout(), m_camera_ptr->GetDescriptorSetLayout(), GetDescriptorSetLayout()},
	    {});

	constexpr uint32_t kChunkVertSpv[] = {
#include <client/shader/chunk.vert.u32>
	};
	constexpr uint32_t kChunkFragSpv[] = {
#include <client/shader/chunk.frag.u32>
	};

	std::shared_ptr<myvk::ShaderModule> vert_shader_module, frag_shader_module;
	vert_shader_module = myvk::ShaderModule::Create(device, kChunkVertSpv, sizeof(kChunkVertSpv));
	frag_shader_module = myvk::ShaderModule::Create(device, kChunkFragSpv, sizeof(kChunkFragSpv));

	std::vector<VkPipelineShaderStageCreateInfo> shader_stages = {
	    vert_shader_module->GetPipelineShaderStageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT),
	    frag_shader_module->GetPipelineShaderStageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT)};

	myvk::GraphicsPipelineState pipeline_state = {};
	pipeline_state.m_vertex_input_state.Enable({{0, sizeof(ChunkMeshVertex), VK_VERTEX_INPUT_RATE_VERTEX}},
	                                           {{0, 0, VK_FORMAT_R32G32_UINT, 0}});
	pipeline_state.m_input_assembly_state.Enable(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
	pipeline_state.m_viewport_state.Enable(1, 1);
	pipeline_state.m_rasterization_state.Initialize(VK_POLYGON_MODE_FILL, VK_FRONT_FACE_COUNTER_CLOCKWISE,
	                                                VK_CULL_MODE_BACK_BIT);
	pipeline_state.m_depth_stencil_state.Enable();
	pipeline_state.m_multisample_state.Enable(VK_SAMPLE_COUNT_1_BIT);
	pipeline_state.m_color_blend_state.Enable(1, VK_FALSE);
	pipeline_state.m_dynamic_state.Enable({VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR});

	m_main_pipeline =
	    myvk::GraphicsPipeline::Create(m_main_pipeline_layout, render_pass, shader_stages, pipeline_state, subpass);
}

/* void ChunkRenderer::CmdRender(const std::shared_ptr<myvk::CommandBuffer> &culling_command_buffer,
                              const std::shared_ptr<myvk::CommandBuffer> &draw_command_buffer, const VkExtent2D &extent,
                              uint32_t current_frame) const {
    auto prepared_clusters = PrepareClusters(current_frame);

    // Culling compute pipeline
    culling_command_buffer->CmdBindPipeline(m_culling_pipeline);
    culling_command_buffer->CmdPushConstants(m_culling_pipeline_layout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(Frustum),
                                             &m_camera_ptr->GetFrustum());
    for (const auto &i : prepared_clusters) {
        culling_command_buffer->CmdBindDescriptorSets({i.m_cluster_ptr->GetFrameDescriptorSet(current_frame)},
                                                      m_culling_pipeline);
        spdlog::info("cluster mesh count: {}", i.m_mesh_count);
        i.m_cluster_ptr->CmdDispatch(culling_command_buffer, i.m_mesh_count);
        i.m_cluster_ptr->CmdBarrier(culling_command_buffer, current_frame);
    }

    // Main graphics pipeline
    draw_command_buffer->CmdBindPipeline(m_main_pipeline);
    VkRect2D scissor = {};
    scissor.extent = extent;
    draw_command_buffer->CmdSetScissor({scissor});
    VkViewport viewport = {};
    viewport.width = (float)extent.width;
    viewport.height = (float)extent.height;
    viewport.maxDepth = 1.0f;
    draw_command_buffer->CmdSetViewport({viewport});

    for (const auto &i : prepared_clusters) {
        draw_command_buffer->CmdBindDescriptorSets({m_texture_ptr->GetDescriptorSet(),
                                                    m_camera_ptr->GetFrameDescriptorSet(current_frame),
                                                    i.m_cluster_ptr->GetFrameDescriptorSet(current_frame)},
                                                   m_main_pipeline);
        i.m_cluster_ptr->CmdDrawIndirect(draw_command_buffer, current_frame, i.m_mesh_count);
    }
} */
void ChunkRenderer::PrepareFrame() {
	uint32_t current_frame = m_depth_ptr->GetCanvasPtr()->GetFrameManagerPtr()->GetCurrentFrame();
	m_frame_prepared_clusters[current_frame] =
	    PrepareClusters(m_depth_ptr->GetCanvasPtr()->GetFrameManagerPtr()->GetCurrentFrame());
}

void ChunkRenderer::CmdDispatch(const std::shared_ptr<myvk::CommandBuffer> &command_buffer) {
	uint32_t current_frame = m_depth_ptr->GetCanvasPtr()->GetFrameManagerPtr()->GetCurrentFrame();
	// Culling compute pipeline
	command_buffer->CmdBindPipeline(m_culling_pipeline);
	for (const auto &i : m_frame_prepared_clusters[current_frame]) {
		command_buffer->CmdBindDescriptorSets({i.m_cluster_ptr->GetFrameDescriptorSet(current_frame),
		                                       m_camera_ptr->GetFrameDescriptorSet(current_frame),
		                                       m_depth_ptr->GetFrameDescriptorSet(current_frame)},
		                                      m_culling_pipeline);
		i.m_cluster_ptr->CmdDispatch(command_buffer, i.m_mesh_count);
		i.m_cluster_ptr->CmdBarrier(command_buffer, current_frame);
	}
}
void ChunkRenderer::CmdDrawIndirect(const std::shared_ptr<myvk::CommandBuffer> &command_buffer) {
	uint32_t current_frame = m_depth_ptr->GetCanvasPtr()->GetFrameManagerPtr()->GetCurrentFrame();
	const VkExtent2D &extent = m_depth_ptr->GetCanvasPtr()->GetFrameManagerPtr()->GetExtent();
	// Main graphics pipeline
	command_buffer->CmdBindPipeline(m_main_pipeline);
	VkRect2D scissor = {};
	scissor.extent = extent;
	command_buffer->CmdSetScissor({scissor});
	VkViewport viewport = {};
	viewport.width = (float)extent.width;
	viewport.height = (float)extent.height;
	viewport.maxDepth = 1.0f;
	command_buffer->CmdSetViewport({viewport});

	for (const auto &i : m_frame_prepared_clusters[current_frame]) {
		command_buffer->CmdBindDescriptorSets({m_texture_ptr->GetDescriptorSet(),
		                                       m_camera_ptr->GetFrameDescriptorSet(current_frame),
		                                       i.m_cluster_ptr->GetFrameDescriptorSet(current_frame)},
		                                      m_main_pipeline);
		i.m_cluster_ptr->CmdDrawIndirect(command_buffer, current_frame, 0, i.m_mesh_count);
	}
}
