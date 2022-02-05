#include <client/Canvas.hpp>

std::shared_ptr<Canvas> Canvas::Create(const std::shared_ptr<myvk::FrameManager> &frame_manager_ptr) {
	std::shared_ptr<Canvas> ret = std::make_shared<Canvas>();
	ret->m_frame_manager_ptr = frame_manager_ptr;
	ret->m_framebuffers.resize(frame_manager_ptr->GetSwapchain()->GetImageCount());

	ret->create_depth_buffer();
	ret->create_render_pass();
	ret->create_framebuffers();
	return ret;
}

void Canvas::Resize() {
	create_depth_buffer();
	create_framebuffers();
}

void Canvas::CmdBeginRenderPass(const std::shared_ptr<myvk::CommandBuffer> &command_buffer, uint32_t image_index,
                                const VkClearColorValue &clear_color) const {
	VkClearValue color_clear_value, depth_clear_value;
	color_clear_value.color = clear_color;
	depth_clear_value.depthStencil = {1.0f, 0u};
	command_buffer->CmdBeginRenderPass(m_render_pass, m_framebuffers[image_index],
	                                   {color_clear_value, depth_clear_value});
}

void Canvas::create_depth_buffer() {
	m_depth_image = myvk::Image::CreateTexture2D(m_frame_manager_ptr->GetSwapchain()->GetDevicePtr(),
	                                             m_frame_manager_ptr->GetSwapchain()->GetExtent(), 1,
	                                             VK_FORMAT_D32_SFLOAT, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);
	m_depth_image_view = myvk::ImageView::Create(m_depth_image, VK_IMAGE_VIEW_TYPE_2D, VK_IMAGE_ASPECT_DEPTH_BIT);
}

void Canvas::create_render_pass() {
	VkAttachmentDescription color_attachment = {};
	color_attachment.format = m_frame_manager_ptr->GetSwapchain()->GetImageFormat();
	color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
	color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	color_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentDescription depth_attachment = {};
	depth_attachment.format = m_depth_image->GetFormat();
	depth_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
	depth_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depth_attachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depth_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	depth_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depth_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	depth_attachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentReference color_attachment_ref = {};
	color_attachment_ref.attachment = 0;
	color_attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentReference depth_attachment_ref = {};
	depth_attachment_ref.attachment = 1;
	depth_attachment_ref.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	std::vector<VkSubpassDescription> subpasses(2);
	subpasses[0].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpasses[0].colorAttachmentCount = 1;
	subpasses[0].pColorAttachments = &color_attachment_ref;
	subpasses[0].pDepthStencilAttachment = &depth_attachment_ref;

	subpasses[1].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpasses[1].colorAttachmentCount = 1;
	subpasses[1].pColorAttachments = &color_attachment_ref;
	subpasses[1].pDepthStencilAttachment = nullptr;

	VkAttachmentDescription attachments[] = {color_attachment, depth_attachment};

	VkRenderPassCreateInfo render_pass_info = {};
	render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	render_pass_info.attachmentCount = 2;
	render_pass_info.pAttachments = attachments;
	render_pass_info.subpassCount = subpasses.size();
	render_pass_info.pSubpasses = subpasses.data();

	std::vector<VkSubpassDependency> subpass_dependencies(3);
	subpass_dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
	subpass_dependencies[0].dstSubpass = 0;
	subpass_dependencies[0].srcStageMask =
	    VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	subpass_dependencies[0].dstStageMask =
	    VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	subpass_dependencies[0].srcAccessMask = 0;
	subpass_dependencies[0].dstAccessMask =
	    VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
	subpass_dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

	subpass_dependencies[1].srcSubpass = 0;
	subpass_dependencies[1].dstSubpass = 1;
	subpass_dependencies[1].srcStageMask =
	    VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	subpass_dependencies[1].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	subpass_dependencies[1].srcAccessMask =
	    VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
	subpass_dependencies[1].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	subpass_dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

	subpass_dependencies[2].srcSubpass = 1;
	subpass_dependencies[2].dstSubpass = VK_SUBPASS_EXTERNAL;
	subpass_dependencies[2].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	subpass_dependencies[2].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	subpass_dependencies[2].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	subpass_dependencies[2].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
	subpass_dependencies[2].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

	render_pass_info.dependencyCount = subpass_dependencies.size();
	render_pass_info.pDependencies = subpass_dependencies.data();

	m_render_pass = myvk::RenderPass::Create(m_frame_manager_ptr->GetSwapchain()->GetDevicePtr(), render_pass_info);
}

void Canvas::create_framebuffers() {
	for (uint32_t i = 0; i < m_framebuffers.size(); ++i)
		m_framebuffers[i] = myvk::Framebuffer::Create(
		    m_render_pass, {m_frame_manager_ptr->GetSwapchainImageViews()[i], m_depth_image_view},
		    m_frame_manager_ptr->GetSwapchain()->GetExtent());
}
