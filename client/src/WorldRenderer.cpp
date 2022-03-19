#include <client/WorldRenderer.hpp>

#include <myvk/ShaderModule.hpp>
#include <spdlog/spdlog.h>

void WorldRenderer::create_descriptors() {
	const std::shared_ptr<myvk::Device> &device = m_transfer_queue->GetDevicePtr();
	uint32_t image_count = m_canvas_ptr->GetFrameManagerPtr()->GetSwapchain()->GetImageCount();
	m_screen_descriptor_pool =
	    myvk::DescriptorPool::Create(device, image_count, {{VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, image_count * 3}});

	m_screen_descriptor_set_layout = myvk::DescriptorSetLayout::Create(
	    device, {
	                {0, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1, VK_SHADER_STAGE_FRAGMENT_BIT},
	                {1, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1, VK_SHADER_STAGE_FRAGMENT_BIT},
	                {2, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1, VK_SHADER_STAGE_FRAGMENT_BIT},
	            });

	m_screen_descriptor_sets.resize(image_count);
	for (auto &i : m_screen_descriptor_sets) {
		i = myvk::DescriptorSet::Create(m_screen_descriptor_pool, m_screen_descriptor_set_layout);
	}
}

void WorldRenderer::update_descriptors() {
	for (size_t i = 0; i < m_screen_descriptor_sets.size(); ++i) {
		auto &set = m_screen_descriptor_sets[i];
		set->UpdateInputAttachment(m_canvas_ptr->GetOpaqueImageView(i), 0);
		set->UpdateInputAttachment(m_canvas_ptr->GetAccumImageView(i), 1);
		set->UpdateInputAttachment(m_canvas_ptr->GetRevealImageView(i), 2);
	}
}

void WorldRenderer::create_pipeline(uint32_t subpass) {
	const std::shared_ptr<myvk::Device> &device = m_transfer_queue->GetDevicePtr();

	m_screen_pipeline_layout = myvk::PipelineLayout::Create(device, {m_screen_descriptor_set_layout}, {});

	constexpr uint32_t kScreenVertSpv[] = {
#include <client/shader/screen.vert.u32>
	};
	constexpr uint32_t kScreenFragSpv[] = {
#include <client/shader/screen.frag.u32>
	};

	std::shared_ptr<myvk::ShaderModule> vert_shader_module, frag_shader_module;
	vert_shader_module = myvk::ShaderModule::Create(device, kScreenVertSpv, sizeof(kScreenVertSpv));
	frag_shader_module = myvk::ShaderModule::Create(device, kScreenFragSpv, sizeof(kScreenFragSpv));

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

	m_screen_pipeline = myvk::GraphicsPipeline::Create(m_screen_pipeline_layout, m_canvas_ptr->GetRenderPass(),
	                                                   shader_stages, pipeline_state, subpass);
}
void WorldRenderer::CmdScreenDraw(const std::shared_ptr<myvk::CommandBuffer> &command_buffer) const {
	const VkExtent2D &extent = m_canvas_ptr->GetFrameManagerPtr()->GetExtent();
	// Main graphics pipeline
	command_buffer->CmdBindPipeline(m_screen_pipeline);
	VkRect2D scissor = {};
	scissor.extent = extent;
	command_buffer->CmdSetScissor({scissor});
	VkViewport viewport = {};
	viewport.width = (float)extent.width;
	viewport.height = (float)extent.height;
	viewport.maxDepth = 1.0f;
	command_buffer->CmdSetViewport({viewport});

	command_buffer->CmdBindDescriptorSets(
	    {m_screen_descriptor_sets[m_canvas_ptr->GetFrameManagerPtr()->GetCurrentImageIndex()]}, m_screen_pipeline);
	command_buffer->CmdDraw(3, 1, 0, 0);
}
