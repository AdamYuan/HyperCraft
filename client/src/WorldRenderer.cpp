#include <client/WorldRenderer.hpp>

#include <myvk/ShaderModule.hpp>
#include <spdlog/spdlog.h>

void WorldRenderer::CmdRenderPass(const std::shared_ptr<myvk::CommandBuffer> &command_buffer) const {
	uint32_t frame = m_frame_manager_ptr->GetCurrentFrame();
	// Generate draw commands for chunks
	m_chunk_renderer->PrepareFrame();
	m_chunk_renderer->CmdDispatch(command_buffer);

	VkClearValue result_clear_value, depth_clear_value, opaque_clear_value, accum_clear_value, reveal_clear_value;
	result_clear_value.color = {0.0f, 0.0f, 0.0f, 0.0f};
	depth_clear_value.depthStencil = {1.0f, 0u};
	opaque_clear_value.color = {0.7, 0.8, 0.96, 1.0};
	accum_clear_value.color = {0.0f, 0.0f, 0.0f, 0.0f};
	reveal_clear_value.color = {1.0f};
	command_buffer->CmdBeginRenderPass(
	    m_render_pass, m_frames[frame].framebuffer,
	    {result_clear_value, depth_clear_value, opaque_clear_value, accum_clear_value, reveal_clear_value});
	{
		// Subpass 0: Opaque
		m_chunk_renderer->CmdOpaqueDrawIndirect(command_buffer);

		// Subpass 1: Transparent
		command_buffer->CmdNextSubpass();
		m_chunk_renderer->CmdTransparentDrawIndirect(command_buffer);

		// Subpass 2: OIT Composite
		command_buffer->CmdNextSubpass();
		m_oit_compositor->CmdDrawPipeline(command_buffer, frame);
	}
	command_buffer->CmdEndRenderPass();
}

void WorldRenderer::create_render_pass() {
	const std::shared_ptr<myvk::Device> &device = m_transfer_queue->GetDevicePtr();

	myvk::RenderPassState state = {3, 5};
	state.RegisterAttachment(0, "ResultImg", VK_FORMAT_B10G11R11_UFLOAT_PACK32, VK_IMAGE_LAYOUT_UNDEFINED,
	                         VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_SAMPLE_COUNT_1_BIT,
	                         VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_STORE);
	state.RegisterAttachment(1, "DepthImg", VK_FORMAT_D32_SFLOAT, VK_IMAGE_LAYOUT_UNDEFINED,
	                         VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_SAMPLE_COUNT_1_BIT,
	                         VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE);
	state.RegisterAttachment(2, "OpaqueImg", VK_FORMAT_B10G11R11_UFLOAT_PACK32, VK_IMAGE_LAYOUT_UNDEFINED,
	                         VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_SAMPLE_COUNT_1_BIT,
	                         VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_DONT_CARE);
	state.RegisterAttachment(3, "AccumImg", VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_LAYOUT_UNDEFINED,
	                         VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_SAMPLE_COUNT_1_BIT,
	                         VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_DONT_CARE);
	state.RegisterAttachment(4, "RevealImg", VK_FORMAT_R8_UNORM, VK_IMAGE_LAYOUT_UNDEFINED,
	                         VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_SAMPLE_COUNT_1_BIT,
	                         VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_DONT_CARE);

	state.RegisterSubpass(0, "OpaquePass")
	    .AddDefaultColorAttachment("OpaqueImg", nullptr)
	    .SetReadWriteDepthStencilAttachment("DepthImg", nullptr);

	state.RegisterSubpass(1, "TransparentPass")
	    .SetReadOnlyDepthStencilAttachment("DepthImg", "OpaquePass")
	    .AddDefaultColorAttachment("AccumImg", nullptr)
	    .AddDefaultColorAttachment("RevealImg", nullptr);

	state.RegisterSubpass(2, "ScreenPass")
	    .AddDefaultColorAttachment("ResultImg", nullptr)
	    // inputs
	    .AddDefaultColorInputAttachment("OpaqueImg", "OpaquePass")
	    .AddDefaultColorInputAttachment("AccumImg", "TransparentPass")
	    .AddDefaultColorInputAttachment("RevealImg", "TransparentPass");

	// Prepare attachments for screen pass input
	state.AddExtraSubpassDependency("ScreenPass", nullptr, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
	                                VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
	                                VK_ACCESS_SHADER_READ_BIT, 0);
	state.AddExtraSubpassDependency("TransparentPass", nullptr,
	                                VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT |
	                                    VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
	                                VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
	                                VK_ACCESS_SHADER_READ_BIT, 0);

	m_render_pass = myvk::RenderPass::Create(device, state);
}

void WorldRenderer::create_frame_descriptors() {
	const std::shared_ptr<myvk::Device> &device = m_transfer_queue->GetDevicePtr();

	m_descriptor_set_layout = myvk::DescriptorSetLayout::Create(
	    device, {{0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT},
	             {1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT}});
	m_descriptor_pool = myvk::DescriptorPool::Create(device, kFrameCount,
	                                                 {{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 2 * kFrameCount}});

	for (auto &frame : m_frames)
		frame.descriptor_set = myvk::DescriptorSet::Create(m_descriptor_pool, m_descriptor_set_layout);
}

void WorldRenderer::create_frame_images() {
	const std::shared_ptr<myvk::Device> &device = m_transfer_queue->GetDevicePtr();
	VkExtent2D extent = m_frame_manager_ptr->GetSwapchain()->GetExtent();

	for (uint32_t i = 0; i < kFrameCount; ++i) {
		auto &frame = m_frames[i];

		frame.result = myvk::Image::CreateTexture2D(device, extent, 1, VK_FORMAT_B10G11R11_UFLOAT_PACK32,
		                                            VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
		frame.result_view = myvk::ImageView::Create(frame.result, VK_IMAGE_VIEW_TYPE_2D, VK_IMAGE_ASPECT_COLOR_BIT);

		frame.depth =
		    myvk::Image::CreateTexture2D(device, extent, 1, VK_FORMAT_D32_SFLOAT,
		                                 VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
		frame.depth_view = myvk::ImageView::Create(frame.depth, VK_IMAGE_VIEW_TYPE_2D, VK_IMAGE_ASPECT_DEPTH_BIT);

		frame.opaque =
		    myvk::Image::CreateTexture2D(device, extent, 1, VK_FORMAT_B10G11R11_UFLOAT_PACK32,
		                                 VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT);
		frame.opaque_view = myvk::ImageView::Create(frame.opaque, VK_IMAGE_VIEW_TYPE_2D, VK_IMAGE_ASPECT_COLOR_BIT);

		frame.accum =
		    myvk::Image::CreateTexture2D(device, extent, 1, VK_FORMAT_R16G16B16A16_SFLOAT,
		                                 VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT);
		frame.accum_view = myvk::ImageView::Create(frame.accum, VK_IMAGE_VIEW_TYPE_2D, VK_IMAGE_ASPECT_COLOR_BIT);

		frame.reveal =
		    myvk::Image::CreateTexture2D(device, extent, 1, VK_FORMAT_R8_UNORM,
		                                 VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT);
		frame.reveal_view = myvk::ImageView::Create(frame.reveal, VK_IMAGE_VIEW_TYPE_2D, VK_IMAGE_ASPECT_COLOR_BIT);

		frame.framebuffer = myvk::Framebuffer::Create(
		    m_render_pass,
		    {frame.result_view, frame.depth_view, frame.opaque_view, frame.accum_view, frame.reveal_view}, extent);

		m_oit_compositor->Update(i, frame.opaque_view, frame.accum_view, frame.reveal_view);

		frame.descriptor_set->UpdateCombinedImageSampler(m_sampler, frame.result_view, 0);
		frame.descriptor_set->UpdateCombinedImageSampler(m_sampler, frame.depth_view, 1);
	}
}
