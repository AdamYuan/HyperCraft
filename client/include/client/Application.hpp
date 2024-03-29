#ifndef HYPERCRAFT_CLIENT_APPLICATION_HPP
#define HYPERCRAFT_CLIENT_APPLICATION_HPP

#include <GLFW/glfw3.h>
#include <myvk/FrameManager.hpp>
#include <myvk/ImGuiRenderer.hpp>

#include <client/Camera.hpp>
#include <client/ClientBase.hpp>
#include <client/GlobalTexture.hpp>
#include <client/WorldRenderer.hpp>
#include <client/WorldWorker.hpp>

#include <client/rg/WorldRenderGraph.hpp>

namespace hc::client {

class Application {
private:
	GLFWwindow *m_window{};

	// vulkan base
	std::shared_ptr<myvk::Instance> m_instance;
	std::shared_ptr<myvk::Surface> m_surface;
	std::shared_ptr<myvk::Device> m_device;
	std::shared_ptr<myvk::Queue> m_main_queue, m_transfer_queue;
	std::shared_ptr<myvk::PresentQueue> m_present_queue;
	std::shared_ptr<myvk::CommandPool> m_main_command_pool;

	// frame objects
	std::shared_ptr<myvk::FrameManager> m_frame_manager;
	myvk::Ptr<rg::WorldRenderGraph> m_world_render_graphs[kFrameCount];

	// game objects and resources
	std::shared_ptr<GlobalTexture> m_global_texture;
	std::shared_ptr<Camera> m_camera;
	std::shared_ptr<World> m_world;
	std::shared_ptr<WorldRenderer> m_world_renderer;
	std::shared_ptr<WorldWorker> m_world_worker;

	// game client
	std::shared_ptr<ClientBase> m_client;

	// Temporal game data TODO: remove
	float m_day_night = 0.0;
	bool m_mouse_captured = false;
	std::optional<ChunkPos3> m_selected_pos, m_outer_selected_pos;
	std::optional<block::Block> m_selected_block;
	void select_block();
	void modify_block();

	void create_glfw_window();
	void create_vulkan_base();
	void create_frame_object();
	void init_imgui();

	void resize(const VkExtent2D &extent);
	void draw_frame(double delta);

	static void glfw_key_callback(GLFWwindow *window, int key, int scancode, int action, int mods);
	static void glfw_framebuffer_resize_callback(GLFWwindow *window, int width, int height);

public:
	Application();
	void Run();
	~Application();
};

} // namespace hc::client

#endif
