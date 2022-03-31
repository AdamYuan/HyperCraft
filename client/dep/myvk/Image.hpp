#ifndef MYVK_IMAGE_HPP
#define MYVK_IMAGE_HPP

#include "ImageBase.hpp"
#include "Queue.hpp"

namespace myvk {
class Image : public ImageBase {
private:
	std::shared_ptr<Device> m_device_ptr;

	VmaAllocation m_allocation{VK_NULL_HANDLE};

public:
	static std::shared_ptr<Image>
	Create(const std::shared_ptr<Device> &device, const VkImageCreateInfo &create_info,
	       VmaAllocationCreateFlags allocation_flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT,
	       VmaMemoryUsage memory_usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE,
	       const std::vector<std::shared_ptr<Queue>> &access_queues = {});

	static uint32_t QueryMipLevel(uint32_t w);

	static uint32_t QueryMipLevel(const VkExtent2D &size);

	static uint32_t QueryMipLevel(const VkExtent3D &size);

	static std::shared_ptr<Image> CreateTexture2D(const std::shared_ptr<Device> &device, const VkExtent2D &size,
	                                              uint32_t mip_level, VkFormat format, VkImageUsageFlags usage,
	                                              const std::vector<std::shared_ptr<Queue>> &access_queue = {});

	static std::shared_ptr<Image> CreateTexture2DArray(const std::shared_ptr<Device> &device, const VkExtent2D &size,
	                                                   uint32_t array_layer, uint32_t mip_level, VkFormat format,
	                                                   VkImageUsageFlags usage,
	                                                   const std::vector<std::shared_ptr<Queue>> &access_queue = {});

	static std::shared_ptr<Image> CreateTexture3D(const std::shared_ptr<Device> &device, const VkExtent3D &size,
	                                              uint32_t mip_level, VkFormat format, VkImageUsageFlags usage,
	                                              const std::vector<std::shared_ptr<Queue>> &access_queue = {});

	const std::shared_ptr<Device> &GetDevicePtr() const override { return m_device_ptr; }

	~Image() override;
};
} // namespace myvk

#endif
