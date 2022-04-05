#include <client/OITCompositor.hpp>

#include <myvk/ShaderModule.hpp>

void OITCompositor::create_descriptor() {
	const std::shared_ptr<myvk::Device> &device = m_render_pass_ptr->GetDevicePtr();
	m_descriptor_pool =
	    myvk::DescriptorPool::Create(device, kFrameCount, {{VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, kFrameCount * 3}});

	m_descriptor_set_layout = myvk::DescriptorSetLayout::Create(
	    device, {
	                {0, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1, VK_SHADER_STAGE_FRAGMENT_BIT},
	                {1, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1, VK_SHADER_STAGE_FRAGMENT_BIT},
	                {2, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1, VK_SHADER_STAGE_FRAGMENT_BIT},
	            });

	for (auto &i : m_descriptor_sets) {
		i = myvk::DescriptorSet::Create(m_descriptor_pool, m_descriptor_set_layout);
	}
}

void OITCompositor::create_pipeline(uint32_t subpass) {
	const std::shared_ptr<myvk::Device> &device = m_render_pass_ptr->GetDevicePtr();

	m_pipeline_layout = myvk::PipelineLayout::Create(device, {m_descriptor_set_layout}, {});

	constexpr uint32_t kQuadVertSpv[] = {
#include <client/shader/quad.vert.u32>
	};
	constexpr uint32_t kOITCompositorFragSpv[] = {
#include <client/shader/oit_compositor.frag.u32>
	};

	std::shared_ptr<myvk::ShaderModule> vert_shader_module, frag_shader_module;
	vert_shader_module = myvk::ShaderModule::Create(device, kQuadVertSpv, sizeof(kQuadVertSpv));
	frag_shader_module = myvk::ShaderModule::Create(device, kOITCompositorFragSpv, sizeof(kOITCompositorFragSpv));

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
	pipeline_state.m_color_blend_state.Enable(1, VK_FALSE);
	pipeline_state.m_dynamic_state.Enable({VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR});

	m_pipeline =
	    myvk::GraphicsPipeline::Create(m_pipeline_layout, m_render_pass_ptr, shader_stages, pipeline_state, subpass);
}
void OITCompositor::Update(uint32_t frame, const std::shared_ptr<myvk::ImageView> &opaque_view,
                           const std::shared_ptr<myvk::ImageView> &accum_view,
                           const std::shared_ptr<myvk::ImageView> &reveal_view) {
	auto &set = m_descriptor_sets[frame];
	m_opaque_views[frame] = opaque_view;
	set->UpdateInputAttachment(opaque_view, 0);
	m_accum_views[frame] = accum_view;
	set->UpdateInputAttachment(accum_view, 1);
	m_reveal_views[frame] = reveal_view;
	set->UpdateInputAttachment(reveal_view, 2);
}

void OITCompositor::CmdDrawPipeline(const std::shared_ptr<myvk::CommandBuffer> &command_buffer, uint32_t frame) const {
	// Main graphics pipeline
	command_buffer->CmdBindPipeline(m_pipeline);
	const VkExtent2D &extent = m_opaque_views[frame]->GetImagePtr()->GetExtent2D();
	VkRect2D scissor = {};
	scissor.extent = extent;
	command_buffer->CmdSetScissor({scissor});
	VkViewport viewport = {};
	viewport.width = (float)extent.width;
	viewport.height = (float)extent.height;
	viewport.maxDepth = 1.0f;
	command_buffer->CmdSetViewport({viewport});

	command_buffer->CmdBindDescriptorSets({m_descriptor_sets[frame]}, m_pipeline);
	command_buffer->CmdDraw(3, 1, 0, 0);
}
