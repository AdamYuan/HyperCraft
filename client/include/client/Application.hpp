#ifndef APPLICATION_HPP
#define APPLICATION_HPP

#include <GLFW/glfw3.h>
#include <myvk/FrameManager.hpp>
#include <myvk/ImGuiRenderer.hpp>

#include <client/Camera.hpp>
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
	myvk::FrameManager m_frame_manager;
	std::vector<std::shared_ptr<myvk::Framebuffer>> m_framebuffers;

	// render pass
	std::shared_ptr<myvk::RenderPass> m_render_pass;
	myvk::ImGuiRenderer m_imgui_renderer;

	std::shared_ptr<myvk::Image> m_depth_image;
	std::shared_ptr<myvk::ImageView> m_depth_image_view;

	// game objects and renderer
	std::shared_ptr<GlobalTexture> m_global_texture;
	std::shared_ptr<Camera> m_camera;
	std::shared_ptr<World> m_world;
	std::shared_ptr<WorldRenderer> m_world_renderer;

	void create_glfw_window();
	void init_imgui();
	void create_vulkan_base();
	void create_depth_buffer();
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
