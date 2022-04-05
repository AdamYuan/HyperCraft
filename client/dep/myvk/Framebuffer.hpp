#ifndef MYVK_FRAMEBUFFER_HPP
#define MYVK_FRAMEBUFFER_HPP

#include "FramebufferBase.hpp"
#include "ImageView.hpp"

#include <memory>
#include <vector>

namespace myvk {
class Framebuffer : public FramebufferBase {
private:
	std::vector<std::shared_ptr<ImageView>> m_image_view_ptrs;

public:
	static std::shared_ptr<Framebuffer> Create(const std::shared_ptr<RenderPass> &render_pass,
	                                           const std::vector<std::shared_ptr<ImageView>> &image_views,
	                                           const VkExtent2D &extent, uint32_t layers = 1);

	static std::shared_ptr<Framebuffer> Create(const std::shared_ptr<RenderPass> &render_pass,
	                                           const std::shared_ptr<ImageView> &image_view);

	const std::vector<std::shared_ptr<ImageView>> &GetImageViewPtrs() const { return m_image_view_ptrs; }

	~Framebuffer() override = default;
};
} // namespace myvk

#endif
