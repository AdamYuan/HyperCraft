#ifndef CUBECRAFT3_POST_PROCESSOR_HPP
#define CUBECRAFT3_POST_PROCESSOR_HPP

#include <myvk/CommandBuffer.hpp>
#include <myvk/FrameManager.hpp>
#include <myvk/ImGuiRenderer.hpp>
#include <myvk/Image.hpp>
#include <myvk/ImageView.hpp>
#include <myvk/ImagelessFramebuffer.hpp>
#include <myvk/RenderPass.hpp>

class WorldRenderer;
class PostProcessor {
private:
	std::shared_ptr<myvk::RenderPass> m_render_pass_ptr;
	std::shared_ptr<myvk::FrameManager> m_frame_manager_ptr;

	std::shared_ptr<WorldRenderer> m_world_renderer_ptr;

	std::shared_ptr<myvk::PipelineLayout> m_pipeline_layout;
	std::shared_ptr<myvk::GraphicsPipeline> m_pipeline;

	void create_pipeline(uint32_t subpass);

public:
	explicit PostProcessor(const std::shared_ptr<WorldRenderer> &world_renderer_ptr,
	                       const std::shared_ptr<myvk::RenderPass> &render_pass_ptr, uint32_t subpass);
	inline static std::unique_ptr<PostProcessor> Create(const std::shared_ptr<WorldRenderer> &world_renderer_ptr,
	                                                    const std::shared_ptr<myvk::RenderPass> &render_pass_ptr,
	                                                    uint32_t subpass) {
		return std::make_unique<PostProcessor>(world_renderer_ptr, render_pass_ptr, subpass);
	}

	void CmdDrawPipeline(const std::shared_ptr<myvk::CommandBuffer> &command_buffer) const;
};

#endif
