#ifndef CUBECRAFT3_CLIENT_CANVAS_HPP
#define CUBECRAFT3_CLIENT_CANVAS_HPP

#include <myvk/CommandBuffer.hpp>
#include <myvk/FrameManager.hpp>
#include <myvk/Framebuffer.hpp>
#include <myvk/Image.hpp>
#include <myvk/ImageView.hpp>
#include <myvk/RenderPass.hpp>

class Canvas {
public:
	static constexpr uint32_t kOpaqueSubpass = 0, kTransparentSubpass = 1, kScreenSubpass = 2, kUISubpass = 3;

private:
	std::shared_ptr<myvk::FrameManager> m_frame_manager_ptr;

	// Render pass
	std::shared_ptr<myvk::RenderPass> m_render_pass;

	// Frame images ( * image_count)
	std::vector<std::shared_ptr<myvk::Framebuffer>> m_framebuffers;

	// Attachments
	struct Attachment {
		std::shared_ptr<myvk::Image> m_image;
		std::shared_ptr<myvk::ImageView> m_image_view;
	};
	std::vector<Attachment> m_depth_buffers, m_opaque_buffers, m_accum_buffers, m_reveal_buffers;

	// Descriptors for opaque, accum, reveal

	void create_depth_buffer();
	void create_opaque_buffer();
	void create_accum_buffer();
	void create_reveal_buffer();
	void create_render_pass();
	void create_framebuffers();

public:
	static std::shared_ptr<Canvas> Create(const std::shared_ptr<myvk::FrameManager> &frame_manager_ptr);
	void Resize();

	inline const std::shared_ptr<myvk::FrameManager> &GetFrameManagerPtr() const { return m_frame_manager_ptr; }
	inline const std::shared_ptr<myvk::RenderPass> &GetRenderPass() const { return m_render_pass; }
	inline const std::shared_ptr<myvk::Image> &GetCurrentDepthImage() const {
		return m_depth_buffers[m_frame_manager_ptr->GetCurrentImageIndex()].m_image;
	}
	inline const std::shared_ptr<myvk::ImageView> &GetOpaqueImageView(uint32_t image_index) const {
		return m_opaque_buffers[image_index].m_image_view;
	}
	inline const std::shared_ptr<myvk::ImageView> &GetAccumImageView(uint32_t image_index) const {
		return m_accum_buffers[image_index].m_image_view;
	}
	inline const std::shared_ptr<myvk::ImageView> &GetRevealImageView(uint32_t image_index) const {
		return m_reveal_buffers[image_index].m_image_view;
	}

	inline const std::shared_ptr<myvk::Framebuffer> &GetCurrentFramebuffer() const {
		return m_framebuffers[m_frame_manager_ptr->GetCurrentImageIndex()];
	}

	void CmdBeginRenderPass(const std::shared_ptr<myvk::CommandBuffer> &command_buffer,
	                        const VkClearColorValue &clear_color = {0.0f, 0.0f, 0.0f, 1.0f}) const;
};

#endif
