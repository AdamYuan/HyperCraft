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
		    VK_FORMAT_B10G11R11_UFLOAT_PACK32,
		    VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT);
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
	myvk::RenderPassState state = {4, 5};
	state.RegisterAttachment(0, "SwapchainImg", m_frame_manager_ptr->GetSwapchain()->GetImageFormat(),
	                         VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_SAMPLE_COUNT_1_BIT,
	                         VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE);
	state.RegisterAttachment(1, "DepthImg", VK_FORMAT_D32_SFLOAT, VK_IMAGE_LAYOUT_UNDEFINED,
	                         VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_SAMPLE_COUNT_1_BIT, VK_ATTACHMENT_LOAD_OP_CLEAR,
	                         VK_ATTACHMENT_STORE_OP_STORE);
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
	    .AddDefaultColorAttachment("SwapchainImg", nullptr)
	    // inputs
	    .AddDefaultColorInputAttachment("OpaqueImg", "OpaquePass")
	    .AddDefaultColorInputAttachment("AccumImg", "TransparentPass")
	    .AddDefaultColorInputAttachment("RevealImg", "TransparentPass");

	state.RegisterSubpass(3, "UIPass").AddDefaultColorAttachment("SwapchainImg", "ScreenPass");

	m_render_pass = myvk::RenderPass::Create(m_frame_manager_ptr->GetSwapchain()->GetDevicePtr(), state);
}

void Canvas::create_framebuffers() {
	for (uint32_t i = 0; i < m_framebuffers.size(); ++i)
		m_framebuffers[i] = myvk::Framebuffer::Create(
		    m_render_pass,
		    {m_frame_manager_ptr->GetSwapchainImageViews()[i], m_depth_buffers[i].m_image_view,
		     m_opaque_buffers[i].m_image_view, m_accum_buffers[i].m_image_view, m_reveal_buffers[i].m_image_view},
		    m_frame_manager_ptr->GetSwapchain()->GetExtent());
}
