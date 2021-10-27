#ifndef APPLICATION_HPP
#define APPLICATION_HPP

#include "../../dep/myvk/FrameManager.hpp"
#include "ImGuiRenderer.hpp"
#include <GLFW/glfw3.h>

class Application {
private:
	GLFWwindow *m_window{};

	// vulkan base
	std::shared_ptr<myvk::Instance> m_instance;
	std::shared_ptr<myvk::Surface> m_surface;
	std::shared_ptr<myvk::Device> m_device;
	std::shared_ptr<myvk::Queue> m_main_queue;
	std::shared_ptr<myvk::PresentQueue> m_present_queue;
	std::shared_ptr<myvk::CommandPool> m_main_command_pool;

	// frame objects
	myvk::FrameManager m_frame_manager;
	std::vector<std::shared_ptr<myvk::Framebuffer>> m_framebuffers;

	// render pass
	std::shared_ptr<myvk::RenderPass> m_render_pass;
	ImGuiRenderer m_imgui_renderer;

	void create_glfw_window();
	void init_imgui();
	void create_vulkan_base();
	void create_render_pass();
	void create_framebuffers();

	void resize(uint32_t width, uint32_t height);
	void draw_frame();

	static void glfw_key_callback(GLFWwindow *window, int key, int scancode, int action, int mods);
	static void glfw_framebuffer_resize_callback(GLFWwindow *window, int width, int height);

public:
	Application();
	void Run();
	~Application();
};

#endif
