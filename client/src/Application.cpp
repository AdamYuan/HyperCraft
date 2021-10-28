#include <client/Application.hpp>
#include <client/ChunkMesher.hpp>
#include <client/Config.hpp>
#include <client/World.hpp>
#include <client/WorldRenderer.hpp>
#include <enet/enet.h>
#include <imgui/imgui_impl_glfw.h>
#include <spdlog/spdlog.h>

#include <random>

void Application::create_glfw_window() {
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

	m_window = glfwCreateWindow(kDefaultWidth, kDefaultHeight, kAppName, nullptr, nullptr);
	glfwSetWindowUserPointer(m_window, this);
	glfwSetKeyCallback(m_window, glfw_key_callback);
	glfwSetFramebufferSizeCallback(m_window, glfw_framebuffer_resize_callback);
}

void Application::init_imgui() {
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui_ImplGlfw_InitForVulkan(m_window, true);
	m_imgui_renderer.Initialize(m_main_command_pool, m_render_pass, 1, kFrameCount);
}

void Application::create_vulkan_base() {
	m_instance = myvk::Instance::CreateWithGlfwExtensions();
	m_surface = myvk::Surface::Create(m_instance, m_window);
	std::vector<std::shared_ptr<myvk::PhysicalDevice>> physical_devices = myvk::PhysicalDevice::Fetch(m_instance);
	if (physical_devices.empty()) {
		spdlog::error("No vulkan physical device available");
		exit(EXIT_FAILURE);
	}
	myvk::DeviceCreateInfo device_create_info = {};
	device_create_info.Initialize(
	    physical_devices[0],
	    myvk::GenericPresentTransferQueueSelector(&m_main_queue, &m_transfer_queue, m_surface, &m_present_queue),
	    {VK_KHR_SWAPCHAIN_EXTENSION_NAME});
	if (!device_create_info.QueueSupport()) {
		spdlog::error("Failed to find queues!");
		exit(EXIT_FAILURE);
	}
	if (!device_create_info.ExtensionSupport()) {
		spdlog::error("Failed to find extension support!");
		exit(EXIT_FAILURE);
	}
	m_device = myvk::Device::Create(device_create_info);
	m_main_command_pool = myvk::CommandPool::Create(m_main_queue);

	spdlog::info("Physical Device: {}", m_device->GetPhysicalDevicePtr()->GetProperties().deviceName);
	spdlog::info("Present Queue: ({}){}, Main Queue: ({}){}, Transfer Queue: ({}){}",      //
	             m_present_queue->GetFamilyIndex(), (void *)m_present_queue->GetHandle(),  // present queue
	             m_main_queue->GetFamilyIndex(), (void *)m_main_queue->GetHandle(),        // main queue
	             m_transfer_queue->GetFamilyIndex(), (void *)m_transfer_queue->GetHandle() // transfer queue
	);

	m_frame_manager.Initialize(m_main_queue, m_present_queue, false, kFrameCount);
	m_frame_manager.SetResizeFunc([&](uint32_t w, uint32_t h) { resize(w, h); });
}

void Application::create_depth_buffer() {
	m_depth_image = myvk::Image::CreateTexture2D(m_device, m_frame_manager.GetSwapchain()->GetExtent(), 1,
	                                             VK_FORMAT_D32_SFLOAT, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);
	m_depth_image_view = myvk::ImageView::Create(m_depth_image, VK_IMAGE_VIEW_TYPE_2D, VK_IMAGE_ASPECT_DEPTH_BIT);
}

void Application::create_render_pass() {
	VkAttachmentDescription color_attachment = {};
	color_attachment.format = m_frame_manager.GetSwapchain()->GetImageFormat();
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

	m_render_pass = myvk::RenderPass::Create(m_device, render_pass_info);
}

void Application::create_framebuffers() {
	if (m_framebuffers.empty())
		m_framebuffers.resize(m_frame_manager.GetSwapchain()->GetImageCount());
	for (uint32_t i = 0; i < m_frame_manager.GetSwapchain()->GetImageCount(); ++i)
		m_framebuffers[i] =
		    myvk::Framebuffer::Create(m_render_pass, {m_frame_manager.GetSwapchainImageViews()[i], m_depth_image_view},
		                              m_frame_manager.GetSwapchain()->GetExtent());
}

void Application::resize(uint32_t width, uint32_t height) {
	m_camera->m_aspect_ratio = (float)width / (float)height;
	create_depth_buffer();
	create_framebuffers();
}

void Application::draw_frame() {
	if (!m_frame_manager.NewFrame())
		return;

	uint32_t image_index = m_frame_manager.GetCurrentImageIndex();
	uint32_t current_frame = m_frame_manager.GetCurrentFrame();

	const std::shared_ptr<myvk::CommandBuffer> &command_buffer = m_frame_manager.GetCurrentCommandBuffer();
	command_buffer->Begin();

	VkClearValue color_clear_value, depth_clear_value;
	color_clear_value.color = {0.0f, 0.0f, 0.0f, 1.0f};
	depth_clear_value.depthStencil = {1.0f, 0u};
	command_buffer->CmdBeginRenderPass(m_render_pass, m_framebuffers[image_index],
	                                   {color_clear_value, depth_clear_value});
	m_camera->UpdateFrameUniformBuffer(current_frame);
	m_world_renderer->CmdDrawPipeline(command_buffer, m_frame_manager.GetSwapchain()->GetExtent(), current_frame);
	command_buffer->CmdNextSubpass();
	m_imgui_renderer.CmdDrawPipeline(command_buffer, current_frame);
	command_buffer->CmdEndRenderPass();

	command_buffer->End();

	m_frame_manager.Render();
}

Application::Application() {
	glfwInit();

	create_glfw_window();
	create_vulkan_base();
	create_depth_buffer();
	create_render_pass();
	create_framebuffers();
	init_imgui();

	m_world = World::Create();
	m_camera = Camera::Create(m_device);
	m_world_renderer = WorldRenderer::Create(m_world, m_camera, m_transfer_queue, m_render_pass, 0);
}

void Application::Run() {
	const auto &chk = m_world->PushChunk({0, 0, 0});
	for (uint32_t i = 0; i < 26; ++i) {
		glm::i16vec3 dp;
		Chunk::NeighbourIndex2CmpXYZ(i, glm::value_ptr(dp));
		m_world->PushChunk(dp);
	}

	/* std::mt19937 gen{std::random_device{}()};
	for (uint32_t i = 0; i < Chunk::kSize * Chunk::kSize * Chunk::kSize; ++i) {
	    chk->SetBlock(gen() % Chunk::kSize, gen() % Chunk::kSize, gen() % Chunk::kSize, Blocks::kSand);
	    m_world->PushWorker(ChunkMesher::Create(chk));
	} */
	m_world->PushWorker(ChunkMesher::Create(chk));

	while (!glfwWindowShouldClose(m_window)) {
		glfwPollEvents();

		m_camera->Control(m_window, 1.0f / ImGui::GetIO().Framerate);

		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		ImGui::Begin("Test");
		ImGui::Button("2333");
		ImGui::Text("%f", ImGui::GetIO().Framerate);
		ImGui::End();

		ImGui::Render();

		draw_frame();
	}
	m_frame_manager.WaitIdle();
	m_world->Join();
}

Application::~Application() {
	glfwDestroyWindow(m_window);
	glfwTerminate();
	enet_deinitialize();
}

void Application::glfw_key_callback(GLFWwindow *window, int key, int scancode, int action, int mods) {
	auto *app = (Application *)glfwGetWindowUserPointer(window);
	/* if (!ImGui::GetCurrentContext()->NavWindow ||
	    (ImGui::GetCurrentContext()->NavWindow->Flags & ImGuiWindowFlags_NoBringToFrontOnFocus)) {
	    if (action == GLFW_PRESS && key == GLFW_KEY_X)
	        app->m_ui_display_flag ^= 1u;
	}*/
}

void Application::glfw_framebuffer_resize_callback(GLFWwindow *window, int width, int height) {
	auto *app = (Application *)glfwGetWindowUserPointer(window);
	app->m_frame_manager.Resize();
}
