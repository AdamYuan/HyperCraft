#ifndef CUBECRAFT3_SCREENRENDERER_HPP
#define CUBECRAFT3_SCREENRENDERER_HPP

#include <client/Config.hpp>
#include <client/DepthHierarchy.hpp>
#include <client/PostProcessor.hpp>

#include <myvk/Buffer.hpp>
#include <myvk/CommandBuffer.hpp>
#include <myvk/DescriptorSet.hpp>
#include <myvk/GraphicsPipeline.hpp>
#include <myvk/ImGuiRenderer.hpp>
#include <myvk/ImagelessFramebuffer.hpp>

class WorldRenderer;
class ScreenRenderer {
private:
	// Pointers
	std::shared_ptr<DepthHierarchy> m_depth_hierarchy_ptr;
	std::shared_ptr<WorldRenderer> m_world_renderer_ptr;
	std::shared_ptr<myvk::FrameManager> m_frame_manager_ptr;

	// Render pass
	std::shared_ptr<myvk::RenderPass> m_render_pass;
	std::shared_ptr<myvk::ImagelessFramebuffer> m_framebuffer;

	// Renderers
	std::unique_ptr<PostProcessor> m_post_processor;
	std::shared_ptr<myvk::ImGuiRenderer> m_imgui_renderer;

	void create_render_pass();
	void create_framebuffer();

public:
	explicit ScreenRenderer(const std::shared_ptr<WorldRenderer> &world_renderer_ptr);
	inline static std::unique_ptr<ScreenRenderer> Create(const std::shared_ptr<WorldRenderer> &world_renderer_ptr) {
		return std::make_unique<ScreenRenderer>(world_renderer_ptr);
	}
	void Resize();
	void CmdRenderPass(const std::shared_ptr<myvk::CommandBuffer> &command_buffer) const;
};

#endif
