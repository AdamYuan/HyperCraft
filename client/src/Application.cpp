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
	m_frame_manager = myvk::FrameManager::Create(m_main_queue, m_present_queue, false, kFrameCount,
	                                             VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT);
	m_frame_manager->SetResizeFunc([this](const VkExtent2D &extent) { resize(extent); });
}

void Application::resize(const VkExtent2D &extent) {
	m_camera->m_aspect_ratio = (float)extent.width / (float)extent.height;
}

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
		(m_selected_pos && m_selected_block)
		    ? world_rg->SetBlockSelection(m_selected_pos.value(), m_selected_block.value())
		    : world_rg->UnsetBlockSelection();
		world_rg->SetDayNight(m_day_night);
		world_rg->UpdateCamera(m_camera);
		world_rg->UpdateDepthHierarchy();
		world_rg->CmdUpdateChunkMesh(command_buffer, 256u);
		world_rg->CmdExecute(command_buffer);
	}
	command_buffer->End();

	m_frame_manager->Render();
}

void Application::select_block() {
	float radius = 10.0f;

	glm::vec3 origin = m_camera->m_position;
	// From "A Fast Voxel Traversal Algorithm for Ray Tracing"
	// by John Amanatides and Andrew Woo, 1987
	// <http://www.cse.yorku.ca/~amana/research/grid.pdf>
	// <http://citeseer.ist.psu.edu/viewdoc/summary?doi=10.1.1.42.3443>
	// Extensions to the described algorithm:
	//   • Imposed a distance limit.
	//   • The face passed through to reach the current cube is provided to
	//     the callback.

	// The foundation of this algorithm is a parameterized representation of
	// the provided ray,
	//                    origin + t * direction,
	// except that t is not actually stored; rather, at any given point in the
	// traversal, we keep track of the *greater* t values which we would have
	// if we took a step sufficient to cross a cube boundary along that axis
	// (i.e. change the integer part of the coordinate) in the variables
	// tMaxX, tMaxY, and tMaxZ.

	const auto int_bound = [](float s, float ds) -> float {
		bool is_int = glm::round(s) == s;
		if (ds < 0 && is_int)
			return 0;
		return (ds > 0 ? (s == 0.0f ? 1.0f : glm::ceil(s)) - s : s - glm::floor(s)) / glm::abs(ds);
	};

	// Cube containing origin point.
	BlockPos3 xyz = glm::floor(origin);
	// Break out direction vector.
	glm::vec3 direction = m_camera->GetViewDirection();
	// Direction to increment x,y,z when stepping.
	BlockPos3 step = glm::sign(direction);
	// See description above. The initial values depend on the fractional
	// part of the origin.
	glm::vec3 t_max =
	    glm::vec3(int_bound(origin.x, direction.x), int_bound(origin.y, direction.y), int_bound(origin.z, direction.z));
	// The change in t when taking a step (always positive).
	glm::vec3 t_delta = (glm::vec3)step / direction;
	// Buffer for reporting faces to the callback.
	BlockPos3 face;

	// Rescale from units of 1 cube-edge to units of 'direction' so we can
	// compare with 't'.
	radius /= glm::sqrt(direction.x * direction.x + direction.y * direction.y + direction.z * direction.z);

	while (true) {
		// Invoke the callback, unless we are not *yet* within the bounds of the
		// world.
		auto opt_block = m_world->GetBlock(xyz);
		if (opt_block && opt_block.value() != block::Blocks::kAir) {
			m_selected_pos = xyz;
			m_outer_selected_pos = xyz + face;
			m_selected_block = opt_block;
			return;
		}

		// tMaxX stores the t-value at which we cross a cube boundary along the
		// X axis, and similarly for Y and Z. Therefore, choosing the least tMax
		// chooses the closest cube boundary. Only the first case of the four
		// has been commented in detail.
		if (t_max.x < t_max.y) {
			if (t_max.x < t_max.z) {
				if (t_max.x > radius)
					break;
				// Update which cube we are now in.
				xyz.x += step.x;
				// Adjust tMaxX to the next X-oriented boundary crossing.
				t_max.x += t_delta.x;
				// Record the normal vector of the cube face we entered.
				face[0] = -step.x;
				face[1] = 0;
				face[2] = 0;
			} else {
				if (t_max.z > radius)
					break;
				xyz.z += step.z;
				t_max.z += t_delta.z;
				face[0] = 0;
				face[1] = 0;
				face[2] = -step.z;
			}
		} else {
			if (t_max.y < t_max.z) {
				if (t_max.y > radius)
					break;
				xyz.y += step.y;
				t_max.y += t_delta.y;
				face[0] = 0;
				face[1] = -step.y;
				face[2] = 0;
			} else {
				// Identical to the second case, repeated for simplicity in
				// the conditionals.
				if (t_max.z > radius)
					break;
				xyz.z += step.z;
				t_max.z += t_delta.z;
				face[0] = 0;
				face[1] = 0;
				face[2] = -step.z;
			}
		}
	}
	m_selected_pos = m_outer_selected_pos = std::nullopt;
	m_selected_block = std::nullopt;
}

void Application::modify_block() {
	static constexpr double kClickInterval = 0.05;
	static double last_click_time = glfwGetTime();
	static bool left_first_click = true, right_first_click = true;
	if (glfwGetMouseButton(m_window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
		right_first_click = true;
		if (m_selected_pos && (left_first_click || glfwGetTime() - last_click_time >= kClickInterval)) {
			m_world->SetBlock(m_selected_pos.value(), block::Blocks::kAir);
			last_click_time = glfwGetTime();
			left_first_click = false;
		}
	} else if (glfwGetMouseButton(m_window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS) {
		left_first_click = true;
		if (m_outer_selected_pos && (right_first_click || glfwGetTime() - last_click_time >= kClickInterval)) {
			m_world->SetBlock(m_outer_selected_pos.value(), block::Blocks::kStone);
			last_click_time = glfwGetTime();
			right_first_click = false;
		}
	} else
		left_first_click = right_first_click = true;
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

	auto concurrency = (int)m_world_worker->GetConcurrency();

	while (!glfwWindowShouldClose(m_window)) {
		glfwPollEvents();

		std::chrono::time_point<std::chrono::steady_clock> cur_time = std::chrono::steady_clock::now();
		std::chrono::duration<double, std::ratio<1, 1>> delta = cur_time - prev_time;
		prev_time = cur_time;
		if (m_mouse_captured) {
			m_camera->MoveControl(m_window, delta.count());
			m_world->SetCenterPos(m_camera->m_position);
			select_block();
			modify_block();
		} else {
			m_selected_pos = m_outer_selected_pos = std::nullopt;
		}

		myvk::ImGuiNewFrame();

		ImGui::Begin("Test");
		ImGui::Text("fps: %f", ImGui::GetIO().Framerate);
		ImGui::Text("frame time: %.1f ms", delta.count() * 1000.0f);
		ImGui::Text("cam: %f %f %f", m_camera->m_position.x, m_camera->m_position.y, m_camera->m_position.z);
		ImGui::Text("pending tasks: %zu", m_world->GetChunkTaskPool().GetPendingTaskCount());
		ImGui::Text("running tasks (approx): %zu", m_world->GetChunkTaskPool().GetRunningTaskCountApprox());
		ImGui::Text("delta: %f", delta.count());
		ImGui::DragFloat("day night", &m_day_night, 0.01f, 0.0f, 1.0f);

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
	if (action != GLFW_PRESS)
		return;
	if (key == GLFW_KEY_P) {
		auto center = (BlockPos3)app->m_camera->m_position;
		std::vector<std::pair<BlockPos3, block::Block>> blocks;
		for (BlockPos1 x = -20; x <= 20; ++x)
			for (BlockPos1 z = -20; z <= 20; ++z)
				blocks.emplace_back(BlockPos3{center.x + x, center.y, center.z + z}, block::Blocks::kGlowstone);
		app->m_world->SetBlockBulk(blocks);
	} else if (key == GLFW_KEY_ESCAPE) {
		app->m_mouse_captured ^= 1;
		spdlog::info("switch {}", app->m_mouse_captured);
		glfwSetInputMode(window, GLFW_CURSOR, app->m_mouse_captured ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
	}
}

void Application::glfw_framebuffer_resize_callback(GLFWwindow *window, int width, int height) {
	auto *app = (Application *)glfwGetWindowUserPointer(window);
	app->m_frame_manager->Resize();
}

} // namespace hc::client
