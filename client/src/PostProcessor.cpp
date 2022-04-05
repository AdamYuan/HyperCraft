#include <client/PostProcessor.hpp>

#include <client/WorldRenderer.hpp>

PostProcessor::PostProcessor(const std::shared_ptr<WorldRenderer> &world_renderer_ptr,
                             const std::shared_ptr<myvk::RenderPass> &render_pass_ptr, uint32_t subpass)
    : m_world_renderer_ptr(world_renderer_ptr), m_frame_manager_ptr(world_renderer_ptr->GetFrameManagerPtr()),
      m_render_pass_ptr(render_pass_ptr) {
	create_pipeline(subpass);
}

void PostProcessor::create_pipeline(uint32_t subpass) {
	const std::shared_ptr<myvk::Device> &device = m_frame_manager_ptr->GetDevicePtr();
	m_pipeline_layout = myvk::PipelineLayout::Create(device, {m_world_renderer_ptr->GetInputDescriptorLayout()}, {});

	constexpr uint32_t kQuadVertSpv[] = {
#include <client/shader/quad.vert.u32>
	};
	constexpr uint32_t kFragSpv[] = {
#include <client/shader/post_process.frag.u32>
	};

	std::shared_ptr<myvk::ShaderModule> vert_shader_module, frag_shader_module;
	vert_shader_module = myvk::ShaderModule::Create(device, kQuadVertSpv, sizeof(kQuadVertSpv));
	frag_shader_module = myvk::ShaderModule::Create(device, kFragSpv, sizeof(kFragSpv));

	std::vector<VkPipelineShaderStageCreateInfo> shader_stages = {
	    vert_shader_module->GetPipelineShaderStageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT),
	    frag_shader_module->GetPipelineShaderStageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT)};

	myvk::GraphicsPipelineState pipeline_state = {};
	pipeline_state.m_vertex_input_state.Enable();
	pipeline_state.m_input_assembly_state.Enable(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
	pipeline_state.m_viewport_state.Enable(1, 1);
	pipeline_state.m_rasterization_state.Initialize(VK_POLYGON_MODE_FILL, VK_FRONT_FACE_COUNTER_CLOCKWISE,
	                                                VK_CULL_MODE_FRONT_BIT);
	pipeline_state.m_multisample_state.Enable(VK_SAMPLE_COUNT_1_BIT);
	pipeline_state.m_color_blend_state.Enable(2, VK_FALSE);
	pipeline_state.m_dynamic_state.Enable({VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR});

	m_pipeline =
	    myvk::GraphicsPipeline::Create(m_pipeline_layout, m_render_pass_ptr, shader_stages, pipeline_state, subpass);
}
void PostProcessor::CmdDrawPipeline(const std::shared_ptr<myvk::CommandBuffer> &command_buffer) const {
	command_buffer->CmdBindPipeline(m_pipeline);
	m_frame_manager_ptr->CmdPipelineSetScreenSize(command_buffer);

	command_buffer->CmdBindDescriptorSets({m_world_renderer_ptr->GetInputDescriptorSet()}, m_pipeline);
	command_buffer->CmdDraw(3, 1, 0, 0);
}
