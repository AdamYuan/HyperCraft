#include "Device.hpp"

namespace myvk {
Device::~Device() {
	if (m_pipeline_cache)
		vkDestroyPipelineCache(m_device, m_pipeline_cache, nullptr);
	if (m_allocator)
		vmaDestroyAllocator(m_allocator);
	if (m_device)
		vkDestroyDevice(m_device, nullptr);
}

std::shared_ptr<Device> Device::Create(const DeviceCreateInfo &device_create_info, void *p_next) {
	std::shared_ptr<Device> ret = std::make_shared<Device>();
	ret->m_physical_device_ptr = device_create_info.m_physical_device_ptr;

	std::vector<VkDeviceQueueCreateInfo> queue_create_infos;
	std::vector<float> queue_priorities;

	device_create_info.enumerate_device_queue_create_infos(&queue_create_infos, &queue_priorities);

	if (ret->create_device(queue_create_infos, device_create_info.m_extensions, p_next) != VK_SUCCESS)
		return nullptr;
	volkLoadDevice(ret->m_device);
	device_create_info.fetch_queues(ret);

	if (device_create_info.m_use_allocator && ret->create_allocator() != VK_SUCCESS)
		return nullptr;

	if (device_create_info.m_use_pipeline_cache && ret->create_pipeline_cache() != VK_SUCCESS)
		return nullptr;

	return ret;
}

VkResult Device::create_device(const std::vector<VkDeviceQueueCreateInfo> &queue_create_infos,
                               const std::vector<const char *> &extensions, void *p_next) {
	VkDeviceCreateInfo create_info = {};
	create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	create_info.pQueueCreateInfos = queue_create_infos.data();
	create_info.queueCreateInfoCount = queue_create_infos.size();
	VkPhysicalDeviceFeatures features = m_physical_device_ptr->GetFeatures();
	features.robustBufferAccess = VK_FALSE; // from ARM/AMD best practice
	create_info.pEnabledFeatures = &features;
	create_info.enabledExtensionCount = extensions.size();
	create_info.ppEnabledExtensionNames = extensions.data();
	create_info.enabledLayerCount = 0;
	create_info.pNext = p_next;

	return vkCreateDevice(m_physical_device_ptr->GetHandle(), &create_info, nullptr, &m_device);
}

VkResult Device::create_allocator() {
	VmaVulkanFunctions vk_funcs = {
		/// Required when using VMA_DYNAMIC_VULKAN_FUNCTIONS.
		vkGetInstanceProcAddr,
		/// Required when using VMA_DYNAMIC_VULKAN_FUNCTIONS.
		vkGetDeviceProcAddr,
		vkGetPhysicalDeviceProperties,
		vkGetPhysicalDeviceMemoryProperties,
		vkAllocateMemory,
		vkFreeMemory,
		vkMapMemory,
		vkUnmapMemory,
		vkFlushMappedMemoryRanges,
		vkInvalidateMappedMemoryRanges,
		vkBindBufferMemory,
		vkBindImageMemory,
		vkGetBufferMemoryRequirements,
		vkGetImageMemoryRequirements,
		vkCreateBuffer,
		vkDestroyBuffer,
		vkCreateImage,
		vkDestroyImage,
		vkCmdCopyBuffer,
#if VMA_DEDICATED_ALLOCATION || VMA_VULKAN_VERSION >= 1001000
		/// Fetch "vkGetBufferMemoryRequirements2" on Vulkan >= 1.1, fetch "vkGetBufferMemoryRequirements2KHR" when
		/// using
		/// VK_KHR_dedicated_allocation extension.
		vkGetBufferMemoryRequirements2KHR,
		/// Fetch "vkGetImageMemoryRequirements 2" on Vulkan >= 1.1, fetch "vkGetImageMemoryRequirements2KHR" when using
		/// VK_KHR_dedicated_allocation extension.
		vkGetImageMemoryRequirements2KHR,
#endif
#if VMA_BIND_MEMORY2 || VMA_VULKAN_VERSION >= 1001000
		/// Fetch "vkBindBufferMemory2" on Vulkan >= 1.1, fetch "vkBindBufferMemory2KHR" when using VK_KHR_bind_memory2
		/// extension.
		vkBindBufferMemory2KHR,
		/// Fetch "vkBindImageMemory2" on Vulkan >= 1.1, fetch "vkBindImageMemory2KHR" when using VK_KHR_bind_memory2
		/// extension.
		vkBindImageMemory2KHR,
#endif
#if VMA_MEMORY_BUDGET || VMA_VULKAN_VERSION >= 1001000
		vkGetPhysicalDeviceMemoryProperties2KHR,
#endif
	};

	VmaAllocatorCreateInfo create_info = {};
	create_info.instance = m_physical_device_ptr->GetInstancePtr()->GetHandle();
	create_info.device = m_device;
	create_info.physicalDevice = m_physical_device_ptr->GetHandle();
	create_info.pVulkanFunctions = &vk_funcs;

	return vmaCreateAllocator(&create_info, &m_allocator);
}

VkResult Device::create_pipeline_cache() {
	VkPipelineCacheCreateInfo create_info = {};
	create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;

	return vkCreatePipelineCache(m_device, &create_info, nullptr, &m_pipeline_cache);
}

VkResult Device::WaitIdle() const { return vkDeviceWaitIdle(m_device); }

} // namespace myvk
