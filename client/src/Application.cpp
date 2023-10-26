#include <client/Application.hpp>
#include <client/Config.hpp>
#include <client/World.hpp>
#include <client/WorldRenderer.hpp>

#include <enet/enet.h>
#include <imgui_impl_glfw.h>
#include <spdlog/spdlog.h>

#include <myvk/ImGuiHelper.hpp>

#include <client/ENetClient.hpp>
#include <client/LocalClient.hpp>
#include <common/WorldDatabase.hpp>

#include <random>

namespace hc::client {

void Application::create_glfw_window() {
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

	m_window = glfwCreateWindow(kDefaultWidth, kDefaultHeight, kAppName, nullptr, nullptr);
	glfwSetWindowUserPointer(m_window, this);
	glfwSetKeyCallback(m_window, glfw_key_callback);
	glfwSetFramebufferSizeCallback(m_window, glfw_framebuffer_resize_callback);
}

void Application::init_imgui() { myvk::ImGuiInit(m_window, m_main_command_pool); }

void Application::create_vulkan_base() {
	m_instance = myvk::Instance::CreateWithGlfwExtensions();
	m_surface = myvk::Surface::Create(m_instance, m_window);
	std::vector<std::shared_ptr<myvk::PhysicalDevice>> physical_devices = myvk::PhysicalDevice::Fetch(m_instance);
	if (physical_devices.empty()) {
		spdlog::error("No vulkan physical device available");
		exit(EXIT_FAILURE);
	}

	myvk::PhysicalDeviceFeatures features = physical_devices[0]->GetDefaultFeatures();
	features.vk12.drawIndirectCount = VK_TRUE;    // for vkCmdDrawIndexedIndirectCount
	features.vk12.samplerFilterMinmax = VK_TRUE;  // for occlusion culling
	features.vk12.imagelessFramebuffer = VK_TRUE; // for imageless framebuffer
	m_device = myvk::Device::Create(
	    physical_devices[0],
	    myvk::GenericPresentTransferQueueSelector(&m_main_queue, &m_transfer_queue, m_surface, &m_present_queue),
	    features, {VK_KHR_SWAPCHAIN_EXTENSION_NAME});

	m_main_command_pool = myvk::CommandPool::Create(m_main_queue);

	spdlog::info("Physical Device: {}", m_device->GetPhysicalDevicePtr()->GetProperties().vk10.deviceName);
	spdlog::info("Present Queue: ({}){}, Main Queue: ({}){}, Transfer Queue: ({}){}",      //
	             m_present_queue->GetFamilyIndex(), (void *)m_present_queue->GetHandle(),  // present queue
	             m_main_queue->GetFamilyIndex(), (void *)m_main_queue->GetHandle(),        // main queue
	             m_transfer_queue->GetFamilyIndex(), (void *)m_transfer_queue->GetHandle() // transfer queue
	);
}

void Application::create_frame_object() {
	m_frame_manager = myvk::FrameManager::Create(m_main_queue, m_present_queue, true, kFrameCount,
	                                             VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT);
	m_frame_manager->SetResizeFunc([this](const VkExtent2D &extent) { resize(extent); });
}

void Application::resize(const VkExtent2D &extent) {
	m_camera->m_aspect_ratio = (float)extent.width / (float)extent.height;
}

float day_night = 0.0;

void Application::draw_frame(double delta) {
	if (!m_frame_manager->NewFrame())
		return;

	uint32_t current_frame = m_frame_manager->GetCurrentFrame();

	std::vector<std::unique_ptr<ChunkMeshPool::LocalUpdate>> post_updates;

	const std::shared_ptr<myvk::CommandBuffer> &command_buffer = m_frame_manager->GetCurrentCommandBuffer();
	command_buffer->Begin();
	{
		m_world_renderer->SetTransferCapacity(8 * 1024 * 1024, delta);

		auto &world_rg = m_world_render_graphs[current_frame];
		world_rg->SetCanvasSize(m_frame_manager->GetExtent());
		world_rg->SetDayNight(day_night);
		world_rg->UpdateCamera(m_camera);
		world_rg->UpdateDepthHierarchy();
		world_rg->CmdUpdateChunkMesh(command_buffer, 256u);
		world_rg->CmdExecute(command_buffer);
	}
	command_buffer->End();

	m_frame_manager->Render();
}

Application::Application() {
	glfwInit();

	create_glfw_window();
	create_vulkan_base();
	create_frame_object();
	init_imgui();

	m_transfer_queue = m_main_queue;

	m_world = World::Create(11, 13);
	m_global_texture = GlobalTexture::Create(m_main_command_pool);
	m_camera = Camera::Create();
	m_camera->m_speed = 32.0f;

	m_world_renderer = WorldRenderer::Create(m_device, m_world);
	m_client = LocalClient::Create(m_world, "world.db"); // Should be placed before worker creation

	m_world_worker = WorldWorker::Create(m_world, std::thread::hardware_concurrency() * 3 / 4);

	for (auto &world_rg : m_world_render_graphs) {
		world_rg = rg::WorldRenderGraph::Create(m_main_queue, m_frame_manager, m_world_renderer, m_global_texture);
		world_rg->SetCanvasSize(m_frame_manager->GetExtent());
	}
}

void Application::Run() {
	std::chrono::time_point<std::chrono::steady_clock> prev_time = std::chrono::steady_clock::now();

	int concurrency = m_world_worker->GetConcurrency();

	while (!glfwWindowShouldClose(m_window)) {
		glfwPollEvents();

		std::chrono::time_point<std::chrono::steady_clock> cur_time = std::chrono::steady_clock::now();
		std::chrono::duration<double, std::ratio<1, 1>> delta = cur_time - prev_time;
		prev_time = cur_time;
		m_camera->Control(m_window, delta.count());
		m_world->SetCenterPos(m_camera->m_position);

		myvk::ImGuiNewFrame();

		ImGui::Begin("Test");
		ImGui::Text("fps: %f", ImGui::GetIO().Framerate);
		ImGui::Text("frame time: %.1f ms", delta.count() * 1000.0f);
		ImGui::Text("cam: %f %f %f", m_camera->m_position.x, m_camera->m_position.y, m_camera->m_position.z);
		ImGui::Text("pending tasks: %zu", m_world->GetChunkTaskPool().GetPendingTaskCount());
		ImGui::Text("running tasks (approx): %zu", m_world->GetChunkTaskPool().GetRunningTaskCountApprox());
		ImGui::Text("delta: %f", delta.count());
		ImGui::DragFloat("day night", &day_night, 0.01f, 0.0f, 1.0f);

		if (ImGui::DragInt("concurrency", &concurrency, 1, 1, (int)std::thread::hardware_concurrency()))
			m_world_worker->Relaunch(std::clamp<std::size_t>(concurrency, 1, std::thread::hardware_concurrency()));

		ImGui::End();

		ImGui::Render();

		draw_frame(delta.count());
	}
	m_frame_manager->WaitIdle();
	m_world_worker->Join();
	m_world_renderer->Join();
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
	if (action == GLFW_PRESS && key == GLFW_KEY_P)
		app->m_world->SetBlock((BlockPos3)app->m_camera->m_position, block::Blocks::kGlowstone);
}

void Application::glfw_framebuffer_resize_callback(GLFWwindow *window, int width, int height) {
	auto *app = (Application *)glfwGetWindowUserPointer(window);
	app->m_frame_manager->Resize();
}

} // namespace hc::client
