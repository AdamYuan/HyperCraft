#include <client/Canvas.hpp>

std::shared_ptr<Canvas> Canvas::Create(const std::shared_ptr<myvk::FrameManager> &frame_manager_ptr) {
	std::shared_ptr<Canvas> ret = std::make_shared<Canvas>();
	ret->m_frame_manager_ptr = frame_manager_ptr;

	ret->m_framebuffers.resize(frame_manager_ptr->GetSwapchain()->GetImageCount());

	ret->m_depth_buffers.resize(frame_manager_ptr->GetSwapchain()->GetImageCount());
	ret->m_opaque_buffers.resize(frame_manager_ptr->GetSwapchain()->GetImageCount());
	ret->m_accum_buffers.resize(frame_manager_ptr->GetSwapchain()->GetImageCount());
	ret->m_reveal_buffers.resize(frame_manager_ptr->GetSwapchain()->GetImageCount());

	ret->create_depth_buffer();
	ret->create_opaque_buffer();
	ret->create_accum_buffer();
	ret->create_reveal_buffer();

	ret->create_render_pass();
	ret->create_framebuffers();
	return ret;
}

void Canvas::Resize() {
	create_depth_buffer();
	create_opaque_buffer();
	create_accum_buffer();
	create_reveal_buffer();

	create_framebuffers();
}

void Canvas::CmdBeginRenderPass(const std::shared_ptr<myvk::CommandBuffer> &command_buffer,
                                const VkClearColorValue &clear_color) const {
	VkClearValue color_clear_value, depth_clear_value, opaque_clear_value, accum_clear_value, reveal_clear_value;
	color_clear_value.color = {0.0f, 0.0f, 0.0f, 0.0f};
	depth_clear_value.depthStencil = {1.0f, 0u};
	opaque_clear_value.color = clear_color;
	accum_clear_value.color = {0.0f, 0.0f, 0.0f, 0.0f};
	reveal_clear_value.color = {1.0f};
	command_buffer->CmdBeginRenderPass(
	    m_render_pass, m_framebuffers[m_frame_manager_ptr->GetCurrentImageIndex()],
	    {color_clear_value, depth_clear_value, opaque_clear_value, accum_clear_value, reveal_clear_value});
}

void Canvas::create_depth_buffer() {
	for (uint32_t i = 0; i < m_framebuffers.size(); ++i) {
		m_depth_buffers[i].m_image = myvk::Image::CreateTexture2D(
		    m_frame_manager_ptr->GetSwapchain()->GetDevicePtr(), m_frame_manager_ptr->GetSwapchain()->GetExtent(), 1,
		    VK_FORMAT_D32_SFLOAT, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT);
		m_depth_buffers[i].m_image_view =
		    myvk::ImageView::Create(m_depth_buffers[i].m_image, VK_IMAGE_VIEW_TYPE_2D, VK_IMAGE_ASPECT_DEPTH_BIT);
	}
}
void Canvas::create_opaque_buffer() {
	for (uint32_t i = 0; i < m_framebuffers.size(); ++i) {
		m_opaque_buffers[i].m_image = myvk::Image::CreateTexture2D(
		    m_frame_manager_ptr->GetSwapchain()->GetDevicePtr(), m_frame_manager_ptr->GetSwapchain()->GetExtent(), 1,
		    VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT);
		m_opaque_buffers[i].m_image_view =
		    myvk::ImageView::Create(m_opaque_buffers[i].m_image, VK_IMAGE_VIEW_TYPE_2D, VK_IMAGE_ASPECT_COLOR_BIT);
	}
}
void Canvas::create_accum_buffer() {
	for (uint32_t i = 0; i < m_framebuffers.size(); ++i) {
		m_accum_buffers[i].m_image = myvk::Image::CreateTexture2D(
		    m_frame_manager_ptr->GetSwapchain()->GetDevicePtr(), m_frame_manager_ptr->GetSwapchain()->GetExtent(), 1,
		    VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT);
		m_accum_buffers[i].m_image_view =
		    myvk::ImageView::Create(m_accum_buffers[i].m_image, VK_IMAGE_VIEW_TYPE_2D, VK_IMAGE_ASPECT_COLOR_BIT);
	}
}
void Canvas::create_reveal_buffer() {
	for (uint32_t i = 0; i < m_framebuffers.size(); ++i) {
		m_reveal_buffers[i].m_image = myvk::Image::CreateTexture2D(
		    m_frame_manager_ptr->GetSwapchain()->GetDevicePtr(), m_frame_manager_ptr->GetSwapchain()->GetExtent(), 1,
		    VK_FORMAT_R8_UNORM, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT);
		m_reveal_buffers[i].m_image_view =
		    myvk::ImageView::Create(m_reveal_buffers[i].m_image, VK_IMAGE_VIEW_TYPE_2D, VK_IMAGE_ASPECT_COLOR_BIT);
	}
}

void Canvas::create_render_pass() {
	VkRenderPassCreateInfo render_pass_info = {VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};

	VkAttachmentDescription color_attachment = {};
	{
		color_attachment.format = m_frame_manager_ptr->GetSwapchain()->GetImageFormat();
		color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
		color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		color_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	}

	VkAttachmentDescription depth_attachment = {};
	{
		depth_attachment.format = VK_FORMAT_D32_SFLOAT;
		depth_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
		depth_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		depth_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		depth_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		depth_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depth_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		depth_attachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	}

	VkAttachmentDescription opaque_attachment = {};
	{
		opaque_attachment.format = VK_FORMAT_R8G8B8A8_UNORM;
		opaque_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
		opaque_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		opaque_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		opaque_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		opaque_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		opaque_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		opaque_attachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	}

	VkAttachmentDescription accum_attachment = {};
	{
		accum_attachment.format = VK_FORMAT_R16G16B16A16_SFLOAT;
		accum_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
		accum_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		accum_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		accum_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		accum_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		accum_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		accum_attachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	}

	VkAttachmentDescription reveal_attachment = {};
	{
		reveal_attachment.format = VK_FORMAT_R8_UNORM;
		reveal_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
		reveal_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		reveal_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		reveal_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		reveal_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		reveal_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		reveal_attachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	}

	// Set attachments
	constexpr uint32_t kColorAttachment = 0, kDepthAttachment = 1, kOpaqueAttachment = 2, kAccumAttachment = 3,
	                   kRevealAttachment = 4;
	std::vector<VkAttachmentDescription> attachments = {color_attachment, depth_attachment, opaque_attachment,
	                                                    accum_attachment, reveal_attachment};
	render_pass_info.attachmentCount = attachments.size();
	render_pass_info.pAttachments = attachments.data();

	std::vector<VkSubpassDescription> subpasses(4);

	VkAttachmentReference p0_color_attachment_ref = {};
	p0_color_attachment_ref.attachment = kOpaqueAttachment;
	p0_color_attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentReference p0_depth_attachment_ref = {};
	p0_depth_attachment_ref.attachment = kDepthAttachment;
	p0_depth_attachment_ref.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	{ // opaque pass

		subpasses[0].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpasses[0].colorAttachmentCount = 1;
		subpasses[0].pColorAttachments = &p0_color_attachment_ref;
		subpasses[0].pDepthStencilAttachment = &p0_depth_attachment_ref;
	}

	VkAttachmentReference p1_color_attachment_refs[] = {{kAccumAttachment, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL},
	                                                    {kRevealAttachment, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL}};
	VkAttachmentReference p1_depth_attachment_ref = {kDepthAttachment,
	                                                 VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};
	{ // transparent pass

		subpasses[1].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpasses[1].colorAttachmentCount = std::size(p1_color_attachment_refs);
		subpasses[1].pColorAttachments = p1_color_attachment_refs;
		subpasses[1].pDepthStencilAttachment = &p1_depth_attachment_ref;
	}

	VkAttachmentReference p2_color_attachment_ref = {};
	p2_color_attachment_ref.attachment = kColorAttachment; // color buffer
	p2_color_attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	// input attachments: opaque, accum, reveal
	VkAttachmentReference p2_input_attachment_refs[] = {{kOpaqueAttachment, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL},
	                                                    {kAccumAttachment, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL},
	                                                    {kRevealAttachment, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL}};
	{ // screen pass
		subpasses[2].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpasses[2].colorAttachmentCount = 1;
		subpasses[2].pColorAttachments = &p2_color_attachment_ref;
		subpasses[2].pDepthStencilAttachment = VK_NULL_HANDLE;
		subpasses[2].inputAttachmentCount = std::size(p2_input_attachment_refs);
		subpasses[2].pInputAttachments = p2_input_attachment_refs;
	}

	VkAttachmentReference p3_color_attachment_ref = {};
	p3_color_attachment_ref.attachment = kColorAttachment;
	p3_color_attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	{ // imgui pass
		subpasses[3].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpasses[3].colorAttachmentCount = 1;
		subpasses[3].pColorAttachments = &p3_color_attachment_ref;
		subpasses[3].pDepthStencilAttachment = nullptr;
	}

	// Set subpasses
	render_pass_info.subpassCount = subpasses.size();
	render_pass_info.pSubpasses = subpasses.data();

	std::vector<VkSubpassDependency> subpass_dependencies(5);
	subpass_dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
	subpass_dependencies[0].dstSubpass = kOpaqueSubpass;
	subpass_dependencies[0].srcStageMask =
	    VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT |
	    VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_TRANSFER_BIT; // TODO: Check it
	subpass_dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
	                                       VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT |
	                                       VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
	subpass_dependencies[0].srcAccessMask = 0;
	subpass_dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
	                                        VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
	                                        VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
	subpass_dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

	subpass_dependencies[1].srcSubpass = kOpaqueSubpass;
	subpass_dependencies[1].dstSubpass = kTransparentSubpass;
	subpass_dependencies[1].srcStageMask = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
	subpass_dependencies[1].dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	subpass_dependencies[1].srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
	subpass_dependencies[1].dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
	subpass_dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

	subpass_dependencies[2].srcSubpass = kOpaqueSubpass;
	subpass_dependencies[2].dstSubpass = kScreenSubpass;
	subpass_dependencies[2].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	subpass_dependencies[2].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	subpass_dependencies[2].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	subpass_dependencies[2].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
	subpass_dependencies[2].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

	subpass_dependencies[3].srcSubpass = kTransparentSubpass;
	subpass_dependencies[3].dstSubpass = kScreenSubpass;
	subpass_dependencies[3].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	subpass_dependencies[3].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	subpass_dependencies[3].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	subpass_dependencies[3].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
	subpass_dependencies[3].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

	subpass_dependencies[4].srcSubpass = kScreenSubpass;
	subpass_dependencies[4].dstSubpass = kUISubpass;
	subpass_dependencies[4].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	subpass_dependencies[4].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	subpass_dependencies[4].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	subpass_dependencies[4].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
	subpass_dependencies[4].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

	// Set dependencies
	render_pass_info.dependencyCount = subpass_dependencies.size();
	render_pass_info.pDependencies = subpass_dependencies.data();

	m_render_pass = myvk::RenderPass::Create(m_frame_manager_ptr->GetSwapchain()->GetDevicePtr(), render_pass_info);
}

void Canvas::create_framebuffers() {
	for (uint32_t i = 0; i < m_framebuffers.size(); ++i)
		m_framebuffers[i] = myvk::Framebuffer::Create(
		    m_render_pass,
		    {m_frame_manager_ptr->GetSwapchainImageViews()[i], m_depth_buffers[i].m_image_view,
		     m_opaque_buffers[i].m_image_view, m_accum_buffers[i].m_image_view, m_reveal_buffers[i].m_image_view},
		    m_frame_manager_ptr->GetSwapchain()->GetExtent());
}
