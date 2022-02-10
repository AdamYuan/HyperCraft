#ifndef CUBECRAFT3_CLIENT_APPLICATION_HPP
#define CUBECRAFT3_CLIENT_APPLICATION_HPP

#include <GLFW/glfw3.h>
#include <myvk/FrameManager.hpp>
#include <myvk/ImGuiRenderer.hpp>

#include <client/Canvas.hpp>

#include <client/Camera.hpp>
#include <client/ClientBase.hpp>
#include <client/DepthHierarchy.hpp>
#include <client/GlobalTexture.hpp>
#include <client/WorldRenderer.hpp>

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
	std::shared_ptr<Canvas> m_canvas;
	std::shared_ptr<DepthHierarchy> m_depth_hierarchy;

	// render pass
	myvk::ImGuiRenderer m_imgui_renderer;
	std::shared_ptr<WorldRenderer> m_world_renderer;

	// game objects and resources
	std::shared_ptr<GlobalTexture> m_global_texture;
	std::shared_ptr<Camera> m_camera;
	std::shared_ptr<World> m_world;

	// game client
	std::shared_ptr<ClientBase> m_client;

	void create_glfw_window();
	void create_vulkan_base();
	void create_frame_object();
	void init_imgui();

	void resize(const myvk::FrameManager &frame_manager);
	void draw_frame();

	static void glfw_key_callback(GLFWwindow *window, int key, int scancode, int action, int mods);
	static void glfw_framebuffer_resize_callback(GLFWwindow *window, int width, int height);

public:
	Application();
	void Run();
	~Application();
};

#endif
