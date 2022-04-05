#include <client/ScreenRenderer.hpp>

#include <client/WorldRenderer.hpp>

ScreenRenderer::ScreenRenderer(const std::shared_ptr<WorldRenderer> &world_renderer_ptr)
    : m_world_renderer_ptr(world_renderer_ptr),
      m_depth_hierarchy_ptr(world_renderer_ptr->GetChunkRenderer()->GetDepthHierarchyPtr()),
      m_frame_manager_ptr(world_renderer_ptr->GetFrameManagerPtr()) {
	create_render_pass();
	create_framebuffer();
	m_post_processor = PostProcessor::Create(world_renderer_ptr, m_render_pass, 0);
	m_imgui_renderer.Initialize(myvk::CommandPool::Create(m_frame_manager_ptr->GetSwapchain()->GetGraphicsQueuePtr()),
	                            m_render_pass, 1, kFrameCount);
}

void ScreenRenderer::create_render_pass() {
	const std::shared_ptr<myvk::Device> &device = m_frame_manager_ptr->GetDevicePtr();

	myvk::RenderPassState state = {2, 2};
	state.RegisterAttachment(0, "SwapchainImg", m_frame_manager_ptr->GetSwapchain()->GetImageFormat(),
	                         VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_SAMPLE_COUNT_1_BIT,
	                         VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_STORE);
	state.RegisterAttachment(1, "HiDepthImg", VK_FORMAT_R32_SFLOAT, VK_IMAGE_LAYOUT_UNDEFINED,
	                         VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_SAMPLE_COUNT_1_BIT,
	                         VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_STORE);

	state.RegisterSubpass(0, "PostProcessPass")
	    .AddDefaultColorAttachment("SwapchainImg", nullptr)
	    .AddColorAttachment("HiDepthImg", VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, nullptr,
	                        VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
	                        VK_ACCESS_SHADER_READ_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, 0);
	state.RegisterSubpass(1, "ImGuiPass").AddDefaultColorAttachment("SwapchainImg", "PostProcessPass");

	state.AddExtraSubpassDependency("PostProcessPass", nullptr, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
	                                VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
	                                VK_ACCESS_SHADER_READ_BIT, 0);

	m_render_pass = myvk::RenderPass::Create(device, state);
}

void ScreenRenderer::create_framebuffer() {
	m_framebuffer = myvk::ImagelessFramebuffer::Create(
	    m_render_pass,
	    {m_frame_manager_ptr->GetSwapchainImages()[0]->GetFramebufferAttachmentImageInfo(),
	     m_depth_hierarchy_ptr->GetAttachmentImageView()->GetImagePtr()->GetFramebufferAttachmentImageInfo()});
}

void ScreenRenderer::Resize() { create_framebuffer(); }

void ScreenRenderer::CmdRenderPass(const std::shared_ptr<myvk::CommandBuffer> &command_buffer) const {
	uint32_t frame = m_frame_manager_ptr->GetCurrentFrame();

	command_buffer->CmdBeginRenderPass(
	    m_render_pass, m_framebuffer,
	    {m_frame_manager_ptr->GetCurrentSwapchainImageView(), m_depth_hierarchy_ptr->GetAttachmentImageView()}, {});
	{
		// Subpass 0: Post Process
		m_post_processor->CmdDrawPipeline(command_buffer);

		// Subpass 1: Transparent
		command_buffer->CmdNextSubpass();
		m_imgui_renderer.CmdDrawPipeline(command_buffer, frame);
	}
	command_buffer->CmdEndRenderPass();
}
