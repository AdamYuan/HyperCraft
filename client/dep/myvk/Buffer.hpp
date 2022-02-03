#ifndef MYVK_BUFFER_HPP
#define MYVK_BUFFER_HPP

#include "BufferBase.hpp"
#include "Queue.hpp"
#include <memory>
#include <vk_mem_alloc.h>
#include <volk.h>

namespace myvk {
class Buffer : public BufferBase {
private:
	std::shared_ptr<Device> m_device_ptr;

	VmaAllocation m_allocation{VK_NULL_HANDLE};

public:
	static std::shared_ptr<Buffer> Create(const std::shared_ptr<Device> &device, const VkBufferCreateInfo &create_info,
	                                      VmaMemoryUsage memory_usage, VmaAllocationCreateFlags allocation_flags,
	                                      const std::vector<std::shared_ptr<Queue>> &access_queues = {});

	static std::shared_ptr<Buffer> Create(const std::shared_ptr<Device> &device, VkDeviceSize size,
	                                      VmaMemoryUsage memory_usage, VkBufferUsageFlags buffer_usage,
	                                      const std::vector<std::shared_ptr<Queue>> &access_queues = {});

	static std::shared_ptr<Buffer> CreateDedicated(const std::shared_ptr<Device> &device, VkDeviceSize size,
	                                               VmaMemoryUsage memory_usage, VkBufferUsageFlags buffer_usage,
	                                               const std::vector<std::shared_ptr<Queue>> &access_queues = {});

	static std::shared_ptr<Buffer> CreateStaging(const std::shared_ptr<Device> &device, VkDeviceSize size,
	                                             const std::vector<std::shared_ptr<Queue>> &access_queues = {});
	template <typename Iter>
	static std::shared_ptr<Buffer> CreateStaging(const std::shared_ptr<Device> &device, Iter begin, Iter end,
	                                             const std::vector<std::shared_ptr<Queue>> &access_queues = {}) {
		using T = typename std::iterator_traits<Iter>::value_type;
		std::shared_ptr<Buffer> ret = CreateStaging(device, (end - begin) * sizeof(T), access_queues);
		ret->UpdateData(begin, end, 0);
		return ret;
	}

	void *Map() const;

	void Unmap() const;

	template <typename Iter> void UpdateData(Iter begin, Iter end, uint32_t byte_offset = 0) const {
		using T = typename std::iterator_traits<Iter>::value_type;
		std::copy(begin, end, (T *)((uint8_t *)Map() + byte_offset));
		Unmap();
	}

	template <typename T> void UpdateData(const T &data, uint32_t byte_offset = 0) const {
		std::copy(&data, &data + 1, (T *)((uint8_t *)Map() + byte_offset));
		Unmap();
	}

	const std::shared_ptr<Device> &GetDevicePtr() const override { return m_device_ptr; }

	~Buffer() override;
};
} // namespace myvk

#endif
