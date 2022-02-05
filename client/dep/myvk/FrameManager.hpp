#ifndef MYVK_FRAME_MANAGER_HPP
#define MYVK_FRAME_MANAGER_HPP

#include "CommandBuffer.hpp"
#include "Fence.hpp"
#include "ImageView.hpp"
#include "Semaphore.hpp"
#include "Swapchain.hpp"
#include "SwapchainImage.hpp"

#include <functional>

namespace myvk {
class FrameManager {
private:
	bool m_resized{false};
	uint32_t m_current_frame{0}, m_current_image_index, m_frame_count;

	std::shared_ptr<Swapchain> m_swapchain;
	std::vector<std::shared_ptr<SwapchainImage>> m_swapchain_images;
	std::vector<std::shared_ptr<ImageView>> m_swapchain_image_views;
	std::vector<Fence *> m_image_fences;
	std::vector<std::shared_ptr<Fence>> m_frame_fences;
	std::vector<std::shared_ptr<Semaphore>> m_render_done_semaphores, m_acquire_done_semaphores;
	std::vector<std::shared_ptr<myvk::CommandBuffer>> m_frame_command_buffers;

	std::function<void(const FrameManager &)> m_resize_func;

	void initialize(const std::shared_ptr<Queue> &graphics_queue, const std::shared_ptr<PresentQueue> &present_queue,
	                bool use_vsync, uint32_t frame_count = 3);
	void recreate_swapchain();

public:
	static std::shared_ptr<FrameManager> Create(const std::shared_ptr<Queue> &graphics_queue,
	                                            const std::shared_ptr<PresentQueue> &present_queue, bool use_vsync,
	                                            uint32_t frame_count = 3);

	void SetResizeFunc(const std::function<void(const FrameManager &)> &resize_func) { m_resize_func = resize_func; }
	void Resize() { m_resized = true; }

	bool NewFrame();
	void Render();

	void WaitIdle() const;

	uint32_t GetCurrentFrame() const { return m_current_frame; }
	uint32_t GetCurrentImageIndex() const { return m_current_image_index; }
	const std::shared_ptr<CommandBuffer> &GetCurrentCommandBuffer() const {
		return m_frame_command_buffers[m_current_frame];
	}

	const std::shared_ptr<Swapchain> &GetSwapchain() const { return m_swapchain; }
	const std::vector<std::shared_ptr<SwapchainImage>> &GetSwapchainImages() const { return m_swapchain_images; }
	const std::vector<std::shared_ptr<ImageView>> &GetSwapchainImageViews() const { return m_swapchain_image_views; }

	VkExtent2D GetExtent() const { return m_swapchain->GetExtent(); }
};
} // namespace myvk

#endif
