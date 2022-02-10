#include <client/Application.hpp>
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
	m_imgui_renderer.Initialize(m_main_command_pool, m_canvas->GetRenderPass(), 1, kFrameCount);
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
	VkPhysicalDeviceVulkan12Features vk12features = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES};
	vk12features.drawIndirectCount = VK_TRUE;   // for vkCmdDrawIndexedIndirectCount
	vk12features.samplerFilterMinmax = VK_TRUE; // for occlusion culling
	m_device = myvk::Device::Create(device_create_info, &vk12features);

	m_main_command_pool = myvk::CommandPool::Create(m_main_queue);

	spdlog::info("Physical Device: {}", m_device->GetPhysicalDevicePtr()->GetProperties().deviceName);
	spdlog::info("Present Queue: ({}){}, Main Queue: ({}){}, Transfer Queue: ({}){}",      //
	             m_present_queue->GetFamilyIndex(), (void *)m_present_queue->GetHandle(),  // present queue
	             m_main_queue->GetFamilyIndex(), (void *)m_main_queue->GetHandle(),        // main queue
	             m_transfer_queue->GetFamilyIndex(), (void *)m_transfer_queue->GetHandle() // transfer queue
	);
}

void Application::create_frame_object() {
	m_frame_manager = myvk::FrameManager::Create(m_main_queue, m_present_queue, false, kFrameCount);
	m_frame_manager->SetResizeFunc([&](const myvk::FrameManager &frame_manager) { resize(frame_manager); });
	m_canvas = Canvas::Create(m_frame_manager);
	m_depth_hierarchy = DepthHierarchy::Create(m_canvas);
}

void Application::resize(const myvk::FrameManager &frame_manager) {
	m_camera->m_aspect_ratio = (float)frame_manager.GetExtent().width / (float)frame_manager.GetExtent().height;
	m_canvas->Resize();
	m_depth_hierarchy->Resize();
}

void Application::draw_frame() {
	if (!m_frame_manager->NewFrame())
		return;

	uint32_t image_index = m_frame_manager->GetCurrentImageIndex();
	uint32_t current_frame = m_frame_manager->GetCurrentFrame();

	m_camera->Update(current_frame);

	const std::shared_ptr<myvk::CommandBuffer> &command_buffer = m_frame_manager->GetCurrentCommandBuffer();
	command_buffer->Begin();
	{
		// Build depth hierarchy for Hi-Z culling (next frame's)
		m_depth_hierarchy->CmdBuild(command_buffer);

		// Generate draw commands for chunks
		m_world_renderer->GetChunkRenderer()->PrepareFrame();
		m_world_renderer->GetChunkRenderer()->CmdDispatch(command_buffer);

		m_canvas->CmdBeginRenderPass(command_buffer);
		{
			// Subpass 0: chunks
			m_world_renderer->GetChunkRenderer()->CmdDrawIndirect(command_buffer);

			// Subpass 1: ImGui
			command_buffer->CmdNextSubpass();
			m_imgui_renderer.CmdDrawPipeline(command_buffer, current_frame);
		}
		command_buffer->CmdEndRenderPass();
	}
	command_buffer->End();

	m_frame_manager->Render();
}

#include <client/ChunkGenerator.hpp>
#include <client/ENetClient.hpp>
#include <client/LocalClient.hpp>
#include <common/WorldDatabase.hpp>

Application::Application() {
	glfwInit();

	create_glfw_window();
	create_vulkan_base();
	create_frame_object();
	init_imgui();

	m_world = World::Create();
	m_global_texture = GlobalTexture::Create(m_main_command_pool);
	m_camera = Camera::Create(m_device);
	m_camera->m_speed = 32.0f;
	m_world_renderer =
	    WorldRenderer::Create(m_world, m_global_texture, m_camera, m_depth_hierarchy, m_transfer_queue, 0);
	m_client = LocalClient::Create(m_world, "world.db");
	// m_client = ENetClient::Create(m_world, "localhost", 60000);
}

void Application::Run() {
	std::chrono::time_point<std::chrono::steady_clock> prev_time = std::chrono::steady_clock::now();

	while (!glfwWindowShouldClose(m_window)) {
		glfwPollEvents();

		std::chrono::time_point<std::chrono::steady_clock> cur_time = std::chrono::steady_clock::now();
		std::chrono::duration<float, std::ratio<1, 1>> delta = cur_time - prev_time;
		prev_time = cur_time;
		m_camera->Control(m_window, delta.count());

		m_world->Update(m_camera->m_position);

		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		ImGui::Begin("Test");
		ImGui::Text("fps: %f", ImGui::GetIO().Framerate);
		ImGui::Text("frame time: %.1f ms", delta.count() * 1000.0f);
		ImGui::Text("cam: %f %f %f", m_camera->m_position.x, m_camera->m_position.y, m_camera->m_position.z);
		ImGui::End();

		ImGui::Render();

		draw_frame();
	}
	m_frame_manager->WaitIdle();
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
	app->m_frame_manager->Resize();
}
