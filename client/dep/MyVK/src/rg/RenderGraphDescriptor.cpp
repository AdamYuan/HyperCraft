#include "RenderGraphDescriptor.hpp"

#include <algorithm>

#include "VkHelper.hpp"
#include "myvk_rg/_details_/Input.hpp"

namespace myvk_rg::_details_ {

void RenderGraphDescriptor::Create(const myvk::Ptr<myvk::Device> &device, const RenderGraphResolver &resolved) {
	m_device_ptr = device;

	m_pass_descriptors.clear();
	m_pass_descriptors.resize(resolved.GetPassNodeCount());

	std::unordered_map<VkDescriptorType, uint32_t> descriptor_type_counts;

	std::vector<myvk::Ptr<myvk::DescriptorSetLayout>> descriptor_set_layouts;

	for (uint32_t i = 0; i < resolved.GetPassNodeCount(); ++i) {
		const PassBase *pass = resolved.GetPassNode(i).pass;
		PassDescriptor &pass_desc = m_pass_descriptors[i];

		if (pass->m_p_descriptor_set_data == nullptr || pass->m_p_descriptor_set_data->m_bindings.empty())
			continue;

		const auto &binding_map = pass->m_p_descriptor_set_data->m_bindings;
		std::vector<VkDescriptorSetLayoutBinding> bindings;
		bindings.reserve(binding_map.size());

		std::vector<VkSampler> immutable_samplers;
		{ // Ensure vector reserved size is larger than possible sampler count
			std::size_t sampler_reserve_size = 0;
			for (const auto &binding_data : binding_map)
				sampler_reserve_size += binding_data.second.size();
			immutable_samplers.reserve(sampler_reserve_size);
		}

		for (const auto &binding_data : binding_map) {
			const auto &array = binding_data.second;
			assert(!array.empty());
			if (array.empty())
				continue;

			bindings.emplace_back();
			VkDescriptorSetLayoutBinding &info = bindings.back();
			info.binding = binding_data.first;
			info.descriptorType = UsageGetDescriptorType(array.front().GetInputPtr()->GetUsage());
			info.descriptorCount = array.size();
			info.stageFlags = VkShaderStagesFromVkPipelineStages(array.front().GetInputPtr()->GetUsagePipelineStages());

			// collect immutable samplers
			if (info.descriptorType == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER) {
				info.pImmutableSamplers = immutable_samplers.data() + immutable_samplers.size();
				for (const auto &element : array)
					immutable_samplers.push_back(element.GetVkSampler()->GetHandle());
			}

			for (uint32_t array_element = 0; array_element < array.size(); ++array_element) {
				const auto set_resource_binding = [&pass_desc, &binding_data, array_element,
				                                   binding = info.binding](const auto *resource) {
					const Input *p_input = binding_data.second[array_element].GetInputPtr();
					DescriptorKey key = {binding, array_element};
					using Trait = ResourceVisitorTrait<decltype(resource)>;
					if constexpr (Trait::kType == ResourceType::kImage) {
						if constexpr (Trait::kIsInternal)
							pass_desc.int_image_bindings[key] = {resource, p_input};
						else if constexpr (Trait::kIsLastFrame)
							pass_desc.lf_image_bindings[key] = {resource, p_input};
						else if constexpr (Trait::kIsExternal) {
							if (resource->IsStatic())
								pass_desc.static_ext_image_bindings[key] = {resource, p_input};
							else
								pass_desc.dynamic_ext_image_bindings[key] = {resource, p_input};
						} else
							assert(false);
					} else {
						if constexpr (Trait::kIsInternal)
							pass_desc.int_buffer_bindings[key] = {resource, p_input};
						else if constexpr (Trait::kIsLastFrame)
							pass_desc.lf_buffer_bindings[key] = {resource, p_input};
						else if constexpr (Trait::kIsExternal) {
							if (resource->IsStatic())
								pass_desc.static_ext_buffer_bindings[key] = {resource, p_input};
							else
								pass_desc.dynamic_ext_buffer_bindings[key] = {resource, p_input};
						} else
							assert(false);
					}
				};

				array[array_element].GetInputPtr()->GetResource()->Visit([&set_resource_binding](const auto *resource) {
					if constexpr (ResourceVisitorTrait<decltype(resource)>::kIsAlias)
						resource->GetPointedResource()->Visit(set_resource_binding);
					else
						set_resource_binding(resource);
				});
			}

			descriptor_type_counts[info.descriptorType] += array.size();
		}
		descriptor_set_layouts.emplace_back(myvk::DescriptorSetLayout::Create(device, bindings));
		// TODO: DescriptorSetLayout Create Callbacks
	}

	// If no descriptors are used, just return
	if (descriptor_type_counts.empty())
		return;

	myvk::Ptr<myvk::DescriptorPool> descriptor_pool;
	{
		std::vector<VkDescriptorPoolSize> pool_sizes;
		pool_sizes.reserve(descriptor_type_counts.size());
		for (const auto &it : descriptor_type_counts) {
			pool_sizes.emplace_back();
			VkDescriptorPoolSize &size = pool_sizes.back();
			size.type = it.first;
			size.descriptorCount = it.second << 1u;
		}
		descriptor_pool = myvk::DescriptorPool::Create(device, descriptor_set_layouts.size() << 1u, pool_sizes);
	}

	std::vector<myvk::Ptr<myvk::DescriptorSet>> descriptor_sets =
	    myvk::DescriptorSet::CreateMultiple(descriptor_pool, descriptor_set_layouts);

	for (uint32_t i = 0, s = 0; i < resolved.GetPassNodeCount(); ++i) {
		const PassBase *pass = resolved.GetPassNode(i).pass;

		if (pass->m_p_descriptor_set_data == nullptr || pass->m_p_descriptor_set_data->m_bindings.empty())
			continue;

		m_pass_descriptors[i].sets[0] = descriptor_sets[s++];
	}

#ifdef MYVK_RG_DEBUG
	printf("Descriptor Created\n");
#endif
}

struct DescriptorWriter {
	std::vector<VkWriteDescriptorSet> writes;
	std::list<VkDescriptorImageInfo> image_infos;
	std::list<VkDescriptorBufferInfo> buffer_infos;

	inline void push_buffer_write(const myvk::Ptr<myvk::DescriptorSet> &set, uint32_t binding, uint32_t array_element,
	                              const myvk::Ptr<myvk::BufferBase> &buffer, const Input *p_input) {
		writes.push_back({VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET});
		VkWriteDescriptorSet &write = writes.back();

		write.descriptorCount = 1u;
		write.descriptorType = UsageGetDescriptorType(p_input->GetUsage());
		write.dstBinding = binding;
		write.dstArrayElement = array_element;
		write.dstSet = set->GetHandle();

		buffer_infos.emplace_back();
		VkDescriptorBufferInfo &buffer_info = buffer_infos.back();
		buffer_info.buffer = buffer->GetHandle();
		buffer_info.offset = 0;
		buffer_info.range = buffer->GetSize();

		write.pBufferInfo = &buffer_info;
	}

	inline void push_image_write(const myvk::Ptr<myvk::DescriptorSet> &set, uint32_t binding, uint32_t array_element,
	                             const myvk::Ptr<myvk::ImageView> &image_view, const Input *p_input) {
		writes.push_back({VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET});
		VkWriteDescriptorSet &write = writes.back();

		write.descriptorCount = 1u;
		write.descriptorType = UsageGetDescriptorType(p_input->GetUsage());
		write.dstBinding = binding;
		write.dstArrayElement = array_element;
		write.dstSet = set->GetHandle();

		image_infos.emplace_back();
		VkDescriptorImageInfo &image_info = image_infos.back();
		image_info.imageLayout = UsageGetImageLayout(p_input->GetUsage());
		image_info.imageView = image_view->GetHandle();

		write.pImageInfo = &image_info;
	}
};

void RenderGraphDescriptor::PreBind(const RenderGraphAllocator &allocated) {
	DescriptorWriter writer{};

	for (auto &pass_desc : m_pass_descriptors) {
		if (pass_desc.sets[1] == nullptr) {
			// Query double_set
			const auto query_double_set = [&allocated](const auto &binding_map) {
				return std::any_of(binding_map.begin(), binding_map.end(), [allocated](const auto &b) {
					if constexpr (ResourceTrait<decltype(b.second.resource)>::kIsImage)
						return allocated.GetIntImageAlloc(b.second.resource).double_buffering;
					else if constexpr (ResourceTrait<decltype(b.second.resource)>::kIsBuffer)
						return allocated.GetIntBufferAlloc(b.second.resource).double_buffering;
					return false;
				});
			};
			bool double_set =
			    query_double_set(pass_desc.int_image_bindings) || query_double_set(pass_desc.int_buffer_bindings) ||
			    query_double_set(pass_desc.lf_image_bindings) || query_double_set(pass_desc.lf_buffer_bindings);

			if (double_set) {
				pass_desc.sets[1] = myvk::DescriptorSet::Create(pass_desc.sets[0]->GetDescriptorPoolPtr(),
				                                                pass_desc.sets[0]->GetDescriptorSetLayoutPtr());
			} else
				pass_desc.sets[1] = pass_desc.sets[0];
		}

		const auto write_int_lf_descriptors = [&writer, &pass_desc, &allocated](bool flip = false) {
			for (const auto &b : pass_desc.int_image_bindings)
				writer.push_image_write(pass_desc.sets[flip], b.first.binding, b.first.array_element,
				                        allocated.GetVkImageView(b.second.resource, flip), b.second.p_input);
			for (const auto &b : pass_desc.lf_image_bindings)
				writer.push_image_write(pass_desc.sets[flip], b.first.binding, b.first.array_element,
				                        allocated.GetVkImageView(b.second.resource, flip), b.second.p_input);
			for (const auto &b : pass_desc.int_buffer_bindings)
				writer.push_buffer_write(pass_desc.sets[flip], b.first.binding, b.first.array_element,
				                         allocated.GetVkBuffer(b.second.resource, flip), b.second.p_input);
			for (const auto &b : pass_desc.lf_buffer_bindings)
				writer.push_buffer_write(pass_desc.sets[flip], b.first.binding, b.first.array_element,
				                         allocated.GetVkBuffer(b.second.resource, flip), b.second.p_input);
			for (const auto &b : pass_desc.static_ext_image_bindings)
				writer.push_image_write(pass_desc.sets[flip], b.first.binding, b.first.array_element,
				                        b.second.resource->GetVkImageView(), b.second.p_input);
			for (const auto &b : pass_desc.static_ext_buffer_bindings)
				writer.push_buffer_write(pass_desc.sets[flip], b.first.binding, b.first.array_element,
				                         b.second.resource->GetVkBuffer(), b.second.p_input);
		};

		write_int_lf_descriptors();
		if (pass_desc.sets[1] != pass_desc.sets[0])
			write_int_lf_descriptors(true);
	}
	if (writer.writes.empty())
		return;

#ifdef MYVK_RG_DEBUG
	printf("Pre-bind descriptors with %zu writes\n", writer.writes.size());
#endif
	vkUpdateDescriptorSets(m_device_ptr->GetHandle(), writer.writes.size(), writer.writes.data(), 0, nullptr);
}

void RenderGraphDescriptor::ExecutionBind(bool flip) {
	DescriptorWriter writer{};

	for (auto &pass_desc : m_pass_descriptors) {
		const auto write_ext_descriptors = [&writer, &pass_desc](bool flip = false) {
			for (const auto &b : pass_desc.dynamic_ext_image_bindings)
				writer.push_image_write(pass_desc.sets[flip], b.first.binding, b.first.array_element,
				                        b.second.resource->GetVkImageView(), b.second.p_input);
			for (const auto &b : pass_desc.dynamic_ext_buffer_bindings)
				writer.push_buffer_write(pass_desc.sets[flip], b.first.binding, b.first.array_element,
				                         b.second.resource->GetVkBuffer(), b.second.p_input);
		};
		write_ext_descriptors(flip);
	}
	if (writer.writes.empty())
		return;

#ifdef MYVK_RG_DEBUG
	printf("Bind execution descriptors with %zu writes\n", writer.writes.size());
#endif
	vkUpdateDescriptorSets(m_device_ptr->GetHandle(), writer.writes.size(), writer.writes.data(), 0, nullptr);
}

} // namespace myvk_rg::_details_