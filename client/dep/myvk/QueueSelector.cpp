#include "QueueSelector.hpp"
#include "Surface.hpp"

namespace myvk {

GenericPresentQueueSelector::GenericPresentQueueSelector(std::shared_ptr<Queue> *generic_queue,
                                                         const std::shared_ptr<Surface> &surface,
                                                         std::shared_ptr<PresentQueue> *present_queue)
    : m_generic_queue(generic_queue), m_surface(surface), m_present_queue(present_queue) {}

bool GenericPresentQueueSelector::operator()(
    const std::shared_ptr<PhysicalDevice> &physical_device, std::vector<QueueSelection> *const out_queue_selections,
    std::vector<PresentQueueSelection> *const out_present_queue_selections) const {

	const auto &families = physical_device->GetQueueFamilyProperties();
	if (families.empty())
		return false;

	myvk::PresentQueueSelection present_queue = {m_present_queue, m_surface, UINT32_MAX};
	myvk::QueueSelection generic_queue = {m_generic_queue, UINT32_MAX};

	// generic queue and present queue
	for (uint32_t i = 0; i < families.size(); ++i) {
		VkQueueFlags flags = families[i].queueFlags;
		if ((flags & VK_QUEUE_GRAPHICS_BIT) && (flags & VK_QUEUE_TRANSFER_BIT) && (flags & VK_QUEUE_COMPUTE_BIT)) {
			generic_queue.family = i;
			generic_queue.index_specifier = 0;

			if (physical_device->GetSurfaceSupport(i, present_queue.surface)) {
				present_queue.family = i;
				present_queue.index_specifier = 0;
				break;
			}
		}
	}

	// present queue fallback
	if (present_queue.family == UINT32_MAX)
		for (uint32_t i = 0; i < families.size(); ++i) {
			if (physical_device->GetSurfaceSupport(i, present_queue.surface)) {
				present_queue.family = i;
				present_queue.index_specifier = 0;
				break;
			}
		}

	(*out_queue_selections) = {generic_queue};
	(*out_present_queue_selections) = {present_queue};

	return (~generic_queue.family) && (~present_queue.family);
}

GenericPresentTransferQueueSelector::GenericPresentTransferQueueSelector(std::shared_ptr<Queue> *generic_queue,
                                                                         std::shared_ptr<Queue> *transfer_queue,
                                                                         const std::shared_ptr<Surface> &surface,
                                                                         std::shared_ptr<PresentQueue> *present_queue)
    : m_generic_queue(generic_queue), m_transfer_queue(transfer_queue), m_surface(surface),
      m_present_queue(present_queue) {}

bool GenericPresentTransferQueueSelector::operator()(
    const std::shared_ptr<PhysicalDevice> &physical_device, std::vector<QueueSelection> *const out_queue_selections,
    std::vector<PresentQueueSelection> *const out_present_queue_selections) const {

	const auto &families = physical_device->GetQueueFamilyProperties();
	if (families.empty())
		return false;

	myvk::PresentQueueSelection present_queue = {m_present_queue, m_surface, UINT32_MAX};
	myvk::QueueSelection generic_queue = {m_generic_queue, UINT32_MAX}, transfer_queue = {m_transfer_queue, UINT32_MAX};

	// generic queue and present queue
	for (uint32_t i = 0; i < families.size(); ++i) {
		VkQueueFlags flags = families[i].queueFlags;
		if ((flags & VK_QUEUE_GRAPHICS_BIT) && (flags & VK_QUEUE_TRANSFER_BIT) && (flags & VK_QUEUE_COMPUTE_BIT)) {
			generic_queue.family = i;
			generic_queue.index_specifier = 0;

			// transfer queue fallback
			transfer_queue.family = i;
			transfer_queue.index_specifier = 1;

			if (physical_device->GetSurfaceSupport(i, present_queue.surface)) {
				present_queue.family = i;
				present_queue.index_specifier = 0;
				break;
			}
		}
	}

	// find standalone transfer queue
	for (uint32_t i = 0; i < families.size(); ++i) {
		VkQueueFlags flags = families[i].queueFlags;
		if ((flags & VK_QUEUE_TRANSFER_BIT) && !(flags & VK_QUEUE_GRAPHICS_BIT) && !(flags & VK_QUEUE_COMPUTE_BIT)) {
			transfer_queue.family = i;
			transfer_queue.index_specifier = 0;
		}
	}

	// present queue fallback
	if (present_queue.family == UINT32_MAX)
		for (uint32_t i = 0; i < families.size(); ++i) {
			if (physical_device->GetSurfaceSupport(i, present_queue.surface)) {
				present_queue.family = i;
				present_queue.index_specifier = 0;
				break;
			}
		}

	(*out_queue_selections) = {generic_queue, transfer_queue};
	(*out_present_queue_selections) = {present_queue};

	return (~generic_queue.family) && (~present_queue.family) && (~transfer_queue.family);
}

} // namespace myvk
