#include "RenderGraphLFInit.hpp"

namespace myvk_rg::_details_ {

void RenderGraphLFInit::InitLastFrameResources(const myvk::Ptr<myvk::Queue> &queue, const RenderGraphResolver &resolved,
                                               const RenderGraphScheduler &scheduled,
                                               const RenderGraphAllocator &allocated) {
	std::vector<VkBufferMemoryBarrier2> pre_buffer_barriers, post_buffer_barriers;
	std::vector<VkImageMemoryBarrier2> pre_image_barriers, post_image_barriers;

	for (const auto &dep : scheduled.GetPassDependencies()) {
		if (dep.type != DependencyType::kLastFrame)
			continue;

		dep.resource->Visit([&allocated, &resolved, &pre_image_barriers, &post_image_barriers, &pre_buffer_barriers,
		                     &post_buffer_barriers](const auto &resource) {
			if constexpr (ResourceVisitorTrait<decltype(resource)>::kClass == ResourceClass::kLastFrameImage) {
				auto last_references = RenderGraphScheduler::GetLastReferences<ResourceType::kImage>(
				    resolved.GetIntResourceInfo(resource).last_references);

				VkPipelineStageFlags2 stage_flags = 0;
				VkAccessFlagBits2 access_flags = 0;
				VkImageLayout layout = UsageGetImageLayout(last_references.front().p_input->GetUsage());
				for (const auto &ref : last_references) {
					stage_flags |= ref.p_input->GetUsagePipelineStages();
					auto usage = ref.p_input->GetUsage();
					access_flags |= UsageGetAccessFlags(usage);
					assert(UsageGetImageLayout(usage) == layout);
				}

				VkImageMemoryBarrier2 barrier = {VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2};
				barrier.srcQueueFamilyIndex = barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

				const auto push_barrier = [&resource, &allocated, &barrier](auto &barriers) {
					const myvk::Ptr<myvk::ImageView> &myvk_image_view_0 = allocated.GetVkImageView(resource, 0);
					const myvk::Ptr<myvk::ImageView> &myvk_image_view_1 = allocated.GetVkImageView(resource, 1);
					barrier.subresourceRange = myvk_image_view_0->GetSubresourceRange();
					barrier.image = myvk_image_view_0->GetImagePtr()->GetHandle();
					barriers.push_back(barrier);
					if (myvk_image_view_1 != myvk_image_view_0) {
						barrier.image = myvk_image_view_1->GetImagePtr()->GetHandle();
						barriers.push_back(barrier);
					}
				};

				assert(resource->GetInitTransferFunc());

				{ // Pre barrier
					barrier.srcStageMask = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT;
					barrier.srcAccessMask = 0;
					barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
					barrier.dstStageMask = VK_PIPELINE_STAGE_2_ALL_TRANSFER_BIT;
					barrier.dstAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
					barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
					push_barrier(pre_image_barriers);
				}
				{ // Post barrier
					barrier.srcStageMask = VK_PIPELINE_STAGE_2_ALL_TRANSFER_BIT;
					barrier.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
					barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
					barrier.dstStageMask = stage_flags;
					barrier.dstAccessMask = access_flags;
					barrier.newLayout = layout;
					push_barrier(post_image_barriers);
				}
			}
			if constexpr (ResourceVisitorTrait<decltype(resource)>::kClass == ResourceClass::kLastFrameBuffer) {
				auto last_references = RenderGraphScheduler::GetLastReferences<ResourceType::kBuffer>(
				    resolved.GetIntResourceInfo(resource).last_references);

				VkPipelineStageFlags2 stage_flags = 0;
				VkAccessFlagBits2 access_flags = 0;
				for (const auto &ref : last_references) {
					stage_flags |= ref.p_input->GetUsagePipelineStages();
					access_flags |= UsageGetAccessFlags(ref.p_input->GetUsage());
				}

				VkBufferMemoryBarrier2 barrier = {VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2};
				barrier.srcQueueFamilyIndex = barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

				const auto push_barrier = [&resource, &allocated, &barrier](auto &barriers) {
					const myvk::Ptr<myvk::BufferBase> &myvk_buffer_0 = allocated.GetVkBuffer(resource, 0);
					const myvk::Ptr<myvk::BufferBase> &myvk_buffer_1 = allocated.GetVkBuffer(resource, 1);
					barrier.offset = 0;
					barrier.size = myvk_buffer_0->GetSize();
					barrier.buffer = myvk_buffer_0->GetHandle();
					barriers.push_back(barrier);
					if (myvk_buffer_1 != myvk_buffer_0) {
						barrier.buffer = myvk_buffer_1->GetHandle();
						barriers.push_back(barrier);
					}
				};

				assert(resource->GetInitTransferFunc());

				{ // Pre barrier
					barrier.srcStageMask = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT;
					barrier.srcAccessMask = 0;
					barrier.dstStageMask = VK_PIPELINE_STAGE_2_ALL_TRANSFER_BIT;
					barrier.dstAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
					push_barrier(pre_buffer_barriers);
				}
				{ // Post barrier
					barrier.srcStageMask = VK_PIPELINE_STAGE_2_ALL_TRANSFER_BIT;
					barrier.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
					barrier.dstStageMask = stage_flags;
					barrier.dstAccessMask = access_flags;
					push_barrier(post_buffer_barriers);
				}
			}
		});
	}

	if (pre_image_barriers.empty() && pre_buffer_barriers.empty() && post_image_barriers.empty() &&
	    post_buffer_barriers.empty())
		return;

	myvk::Ptr<myvk::CommandBuffer> command_buffer = myvk::CommandBuffer::Create(myvk::CommandPool::Create(queue));
	command_buffer->Begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
	{ // Pre barriers
		VkDependencyInfo dep_info = {VK_STRUCTURE_TYPE_DEPENDENCY_INFO};
		dep_info.imageMemoryBarrierCount = pre_image_barriers.size();
		dep_info.pImageMemoryBarriers = pre_image_barriers.data();
		dep_info.bufferMemoryBarrierCount = pre_buffer_barriers.size();
		dep_info.pBufferMemoryBarriers = pre_buffer_barriers.data();
		vkCmdPipelineBarrier2(command_buffer->GetHandle(), &dep_info);
	}

	for (const auto &dep : scheduled.GetPassDependencies()) {
		if (dep.type != DependencyType::kLastFrame)
			continue;

		dep.resource->Visit([&allocated, &command_buffer](const auto &resource) {
			if constexpr (ResourceVisitorTrait<decltype(resource)>::kClass == ResourceClass::kLastFrameImage) {
				assert(resource->GetInitTransferFunc());

				const myvk::Ptr<myvk::ImageView> &myvk_image_view_0 = allocated.GetVkImageView(resource, 0);
				resource->GetInitTransferFunc()(command_buffer, myvk_image_view_0);
				const myvk::Ptr<myvk::ImageView> &myvk_image_view_1 = allocated.GetVkImageView(resource, 1);
				if (myvk_image_view_1 != myvk_image_view_0)
					resource->GetInitTransferFunc()(command_buffer, myvk_image_view_1);
			}
			if constexpr (ResourceVisitorTrait<decltype(resource)>::kClass == ResourceClass::kLastFrameBuffer) {
				assert(resource->GetInitTransferFunc());

				const myvk::Ptr<myvk::BufferBase> &myvk_buffer_0 = allocated.GetVkBuffer(resource, 0);
				resource->GetInitTransferFunc()(command_buffer, myvk_buffer_0);
				const myvk::Ptr<myvk::BufferBase> &myvk_buffer_1 = allocated.GetVkBuffer(resource, 1);
				if (myvk_buffer_1 != myvk_buffer_0)
					resource->GetInitTransferFunc()(command_buffer, myvk_buffer_1);
			}
		});
	}
	{ // Post barriers
		VkDependencyInfo dep_info = {VK_STRUCTURE_TYPE_DEPENDENCY_INFO};
		dep_info.imageMemoryBarrierCount = post_image_barriers.size();
		dep_info.pImageMemoryBarriers = post_image_barriers.data();
		dep_info.bufferMemoryBarrierCount = post_buffer_barriers.size();
		dep_info.pBufferMemoryBarriers = post_buffer_barriers.data();
		vkCmdPipelineBarrier2(command_buffer->GetHandle(), &dep_info);
	}

	command_buffer->End();
	auto fence = myvk::Fence::Create(queue->GetDevicePtr());
	command_buffer->Submit(fence);
	fence->Wait();
}

} // namespace myvk_rg::_details_
