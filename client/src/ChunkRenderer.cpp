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

void ChunkRenderer::create_pipeline_layout(const std::shared_ptr<myvk::Device> &device) {
	m_pipeline_layout = myvk::PipelineLayout::Create(
	    device,
	    {m_texture_ptr->GetDescriptorSetLayout(), m_camera_ptr->GetDescriptorSetLayout(), GetDescriptorSetLayout()},
	    {});
}

void ChunkRenderer::create_opaque_pipeline(const std::shared_ptr<myvk::RenderPass> &render_pass, uint32_t subpass) {
	const std::shared_ptr<myvk::Device> &device = render_pass->GetDevicePtr();

	constexpr uint32_t kChunkVertSpv[] = {
#include <client/shader/chunk.vert.u32>
	};
	constexpr uint32_t kChunkOpaqueFragSpv[] = {
#include <client/shader/chunk_opaque.frag.u32>
	};

	std::shared_ptr<myvk::ShaderModule> vert_shader_module, frag_shader_module;
	vert_shader_module = myvk::ShaderModule::Create(device, kChunkVertSpv, sizeof(kChunkVertSpv));
	frag_shader_module = myvk::ShaderModule::Create(device, kChunkOpaqueFragSpv, sizeof(kChunkOpaqueFragSpv));

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
	pipeline_state.m_depth_stencil_state.Enable(VK_TRUE, VK_TRUE);
	pipeline_state.m_multisample_state.Enable(VK_SAMPLE_COUNT_1_BIT);
	pipeline_state.m_color_blend_state.Enable(1, VK_FALSE);
	pipeline_state.m_dynamic_state.Enable({VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR});

	m_opaque_pipeline =
	    myvk::GraphicsPipeline::Create(m_pipeline_layout, render_pass, shader_stages, pipeline_state, subpass);
}

void ChunkRenderer::create_transparent_pipeline(const std::shared_ptr<myvk::RenderPass> &render_pass,
                                                uint32_t subpass) {
	const std::shared_ptr<myvk::Device> &device = render_pass->GetDevicePtr();

	constexpr uint32_t kChunkVertSpv[] = {
#include <client/shader/chunk.vert.u32>
	};
	constexpr uint32_t kChunkTransparentFragSpv[] = {
#include <client/shader/chunk_transparent.frag.u32>
	};

	std::shared_ptr<myvk::ShaderModule> vert_shader_module, frag_shader_module;
	vert_shader_module = myvk::ShaderModule::Create(device, kChunkVertSpv, sizeof(kChunkVertSpv));
	frag_shader_module = myvk::ShaderModule::Create(device, kChunkTransparentFragSpv, sizeof(kChunkTransparentFragSpv));

	std::vector<VkPipelineShaderStageCreateInfo> shader_stages = {
	    vert_shader_module->GetPipelineShaderStageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT),
	    frag_shader_module->GetPipelineShaderStageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT)};

	myvk::GraphicsPipelineState pipeline_state = {};
	pipeline_state.m_vertex_input_state.Enable({{0, sizeof(ChunkMeshVertex), VK_VERTEX_INPUT_RATE_VERTEX}},
	                                           {{0, 0, VK_FORMAT_R32G32_UINT, 0}});
	pipeline_state.m_input_assembly_state.Enable(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
	pipeline_state.m_viewport_state.Enable(1, 1);
	pipeline_state.m_rasterization_state.Initialize(VK_POLYGON_MODE_FILL, VK_FRONT_FACE_COUNTER_CLOCKWISE,
	                                                VK_CULL_MODE_NONE);
	pipeline_state.m_depth_stencil_state.Enable(VK_TRUE, VK_FALSE);
	pipeline_state.m_multisample_state.Enable(VK_SAMPLE_COUNT_1_BIT);
	{
		// WBOIT blend function
		VkPipelineColorBlendAttachmentState accum = {};
		accum.blendEnable = VK_TRUE;
		accum.srcColorBlendFactor = accum.srcAlphaBlendFactor = accum.dstColorBlendFactor = accum.dstAlphaBlendFactor =
		    VK_BLEND_FACTOR_ONE;
		accum.colorBlendOp = accum.alphaBlendOp = VK_BLEND_OP_ADD;
		accum.colorWriteMask =
		    VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

		VkPipelineColorBlendAttachmentState reveal = {};
		reveal.blendEnable = VK_TRUE;
		reveal.srcColorBlendFactor = VK_BLEND_FACTOR_ZERO;
		reveal.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;
		reveal.colorBlendOp = VK_BLEND_OP_ADD;
		reveal.colorWriteMask = VK_COLOR_COMPONENT_R_BIT;
		pipeline_state.m_color_blend_state.Enable({accum, reveal});
	}
	pipeline_state.m_dynamic_state.Enable({VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR});

	m_transparent_pipeline =
	    myvk::GraphicsPipeline::Create(m_pipeline_layout, render_pass, shader_stages, pipeline_state, subpass);
}

/* void ChunkRenderer::CmdRender(const std::shared_ptr<myvk::CommandBuffer> &culling_command_buffer,
                              const std::shared_ptr<myvk::CommandBuffer> &draw_command_buffer, const VkExtent2D &extent,
                              uint32_t frame) const {
    auto prepared_clusters = PrepareClusters(frame);

    // Culling compute pipeline
    culling_command_buffer->CmdBindPipeline(m_culling_pipeline);
    culling_command_buffer->CmdPushConstants(m_culling_pipeline_layout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(Frustum),
                                             &m_camera_ptr->GetFrustum());
    for (const auto &i : prepared_clusters) {
        culling_command_buffer->CmdBindDescriptorSets({i.m_cluster_ptr->GetFrameDescriptorSet(frame)},
                                                      m_culling_pipeline);
        spdlog::info("cluster mesh count: {}", i.m_mesh_count);
        i.m_cluster_ptr->CmdDispatch(culling_command_buffer, i.m_mesh_count);
        i.m_cluster_ptr->CmdBarrier(culling_command_buffer, frame);
    }

    // Main graphics pipeline
    draw_command_buffer->CmdBindPipeline(m_opaque_pipeline);
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
                                                    m_camera_ptr->GetFrameDescriptorSet(frame),
                                                    i.m_cluster_ptr->GetFrameDescriptorSet(frame)},
                                                   m_opaque_pipeline);
        i.m_cluster_ptr->CmdDrawIndirect(draw_command_buffer, frame, i.m_mesh_count);
    }
} */
void ChunkRenderer::PrepareFrame() {
	uint32_t frame = m_frame_manager_ptr->GetCurrentFrame();
	m_frame_prepared_clusters[frame] = PrepareClusters(frame);
}

void ChunkRenderer::CmdDispatch(const std::shared_ptr<myvk::CommandBuffer> &command_buffer) {
	uint32_t frame = m_frame_manager_ptr->GetCurrentFrame();
	// Culling compute pipeline
	command_buffer->CmdBindPipeline(m_culling_pipeline);
	for (const auto &i : m_frame_prepared_clusters[frame]) {
		command_buffer->CmdBindDescriptorSets({i.m_cluster_ptr->GetFrameDescriptorSet(frame),
		                                       m_camera_ptr->GetFrameDescriptorSet(frame),
		                                       m_depth_ptr->GetFrameDescriptorSet(frame)},
		                                      m_culling_pipeline);
		i.m_cluster_ptr->CmdDispatch(command_buffer, i.m_mesh_count);
		i.m_cluster_ptr->CmdBarrier(command_buffer, frame);
	}
}
void ChunkRenderer::CmdOpaqueDrawIndirect(const std::shared_ptr<myvk::CommandBuffer> &command_buffer) {
	uint32_t frame = m_frame_manager_ptr->GetCurrentFrame();
	const VkExtent2D &extent = m_frame_manager_ptr->GetExtent();
	// Main graphics pipeline
	command_buffer->CmdBindPipeline(m_opaque_pipeline);
	VkRect2D scissor = {};
	scissor.extent = extent;
	command_buffer->CmdSetScissor({scissor});
	VkViewport viewport = {};
	viewport.width = (float)extent.width;
	viewport.height = (float)extent.height;
	viewport.maxDepth = 1.0f;
	command_buffer->CmdSetViewport({viewport});

	for (const auto &i : m_frame_prepared_clusters[frame]) {
		command_buffer->CmdBindDescriptorSets({m_texture_ptr->GetDescriptorSet(),
		                                       m_camera_ptr->GetFrameDescriptorSet(frame),
		                                       i.m_cluster_ptr->GetFrameDescriptorSet(frame)},
		                                      m_opaque_pipeline);
		i.m_cluster_ptr->CmdDrawIndirect(command_buffer, frame, 0, i.m_mesh_count);
	}
}
void ChunkRenderer::CmdTransparentDrawIndirect(const std::shared_ptr<myvk::CommandBuffer> &command_buffer) {
	uint32_t frame = m_frame_manager_ptr->GetCurrentFrame();
	// Main graphics pipeline

	command_buffer->CmdBindPipeline(m_transparent_pipeline);
	m_frame_manager_ptr->CmdPipelineSetScreenSize(command_buffer);

	for (const auto &i : m_frame_prepared_clusters[frame]) {
		command_buffer->CmdBindDescriptorSets({m_texture_ptr->GetDescriptorSet(),
		                                       m_camera_ptr->GetFrameDescriptorSet(frame),
		                                       i.m_cluster_ptr->GetFrameDescriptorSet(frame)},
		                                      m_opaque_pipeline);
		i.m_cluster_ptr->CmdDrawIndirect(command_buffer, frame, 1, i.m_mesh_count);
	}
}
