#ifndef MYVK_IMAGELESS_FRAMEBUFFER_HPP
#define MYVK_IMAGELESS_FRAMEBUFFER_HPP

#include "FramebufferBase.hpp"

namespace myvk {
class ImagelessFramebuffer : public FramebufferBase {
private:
public:
	static std::shared_ptr<ImagelessFramebuffer>
	Create(const std::shared_ptr<RenderPass> &render_pass,
	       const std::vector<VkFramebufferAttachmentImageInfo> &attachment_image_infos, const VkExtent2D &extent,
	       uint32_t layers = 1);

	static std::shared_ptr<ImagelessFramebuffer>
	Create(const std::shared_ptr<RenderPass> &render_pass,
	       const std::vector<VkFramebufferAttachmentImageInfo> &attachment_image_infos);

	~ImagelessFramebuffer() override = default;
};
} // namespace myvk

#endif
