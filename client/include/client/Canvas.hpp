#ifndef CUBECRAFT3_CLIENT_CANVAS_HPP
#define CUBECRAFT3_CLIENT_CANVAS_HPP

#include <myvk/CommandBuffer.hpp>
#include <myvk/FrameManager.hpp>
#include <myvk/Framebuffer.hpp>
#include <myvk/Image.hpp>
#include <myvk/ImageView.hpp>
#include <myvk/RenderPass.hpp>

class Canvas {
private:
	std::shared_ptr<myvk::Image> m_depth_image;
	std::shared_ptr<myvk::ImageView> m_depth_image_view;

	std::shared_ptr<myvk::RenderPass> m_render_pass;

	std::vector<std::shared_ptr<myvk::Framebuffer>> m_framebuffers;

	void create_depth_buffer(const myvk::FrameManager &frame_manager);
	void create_render_pass(const myvk::FrameManager &frame_manager);
	void create_framebuffers(const myvk::FrameManager &frame_manager);

public:
	void Initialize(const myvk::FrameManager &frame_manager);
	void Resize(const myvk::FrameManager &frame_manager);

	inline const std::shared_ptr<myvk::RenderPass> &GetRenderPass() const { return m_render_pass; }
	inline const std::shared_ptr<myvk::Framebuffer> &GetFramebuffer(uint32_t image_index) const {
		return m_framebuffers[image_index];
	}

	void CmdBeginRenderPass(const std::shared_ptr<myvk::CommandBuffer> &command_buffer, uint32_t image_index,
	                        const VkClearColorValue &clear_color = {0.0f, 0.0f, 0.0f, 1.0f}) const;
};

#endif
