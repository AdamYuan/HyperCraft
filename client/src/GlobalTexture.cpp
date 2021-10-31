#include <client/GlobalTexture.hpp>
#include <client/texture/block_texture_png.inl>

#include <myvk/Buffer.hpp>
#include <myvk/CommandBuffer.hpp>

#include <stb_image.h>

std::shared_ptr<GlobalTexture> GlobalTexture::Create(const std::shared_ptr<myvk::CommandPool> &command_pool) {
	std::shared_ptr<GlobalTexture> ret = std::make_shared<GlobalTexture>();
	ret->create_block_texture(command_pool);
	ret->create_descriptors(command_pool->GetDevicePtr());
	return ret;
}

void GlobalTexture::create_block_texture(const std::shared_ptr<myvk::CommandPool> &command_pool) {
	int x, y, c;

	stbi_uc *img = stbi_load_from_memory(kBlockTexturePng, sizeof(kBlockTexturePng), &x, &y, &c, 4);
	std::shared_ptr<myvk::Buffer> block_staging =
	    myvk::Buffer::CreateStaging(command_pool->GetDevicePtr(), img, img + x * y * 4);

	m_block_texture = myvk::Image::CreateTexture2DArray(command_pool->GetDevicePtr(),
	                                                    {
	                                                        (uint32_t)x,
	                                                        (uint32_t)x,
	                                                    },
	                                                    y / x, myvk::Image::QueryMipLevel(x), VK_FORMAT_R8G8B8A8_UNORM,
	                                                    VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
	                                                        VK_IMAGE_USAGE_TRANSFER_DST_BIT);
	m_block_view = myvk::ImageView::Create(m_block_texture, VK_IMAGE_VIEW_TYPE_2D_ARRAY);
	m_block_sampler =
	    myvk::Sampler::Create(command_pool->GetDevicePtr(), VK_FILTER_NEAREST, VK_SAMPLER_ADDRESS_MODE_REPEAT,
	                          VK_SAMPLER_MIPMAP_MODE_LINEAR, m_block_texture->GetMipLevels());

	VkBufferImageCopy region = {};
	region.bufferOffset = 0;
	region.bufferRowLength = 0;
	region.bufferImageHeight = 0;
	region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.imageSubresource.mipLevel = 0;
	region.imageSubresource.baseArrayLayer = 0;
	region.imageSubresource.layerCount = m_block_texture->GetArrayLayers();
	region.imageOffset = {0, 0, 0};
	region.imageExtent = {(uint32_t)x, (uint32_t)x, 1};

	std::shared_ptr<myvk::CommandBuffer> command_buffer = myvk::CommandBuffer::Create(command_pool);

	command_buffer->Begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
	command_buffer->CmdPipelineBarrier(VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, {}, {},
	                                   m_block_texture->GetDstMemoryBarriers({region}, 0, VK_ACCESS_TRANSFER_WRITE_BIT,
	                                                                         VK_IMAGE_LAYOUT_UNDEFINED,
	                                                                         VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL));
	command_buffer->CmdCopy(block_staging, m_block_texture, {region});
	command_buffer->CmdPipelineBarrier(VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, {}, {},
	                                   m_block_texture->GetDstMemoryBarriers(
	                                       {region}, VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT,
	                                       VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL));
	command_buffer->CmdGenerateMipmap2D(m_block_texture, VK_PIPELINE_STAGE_TRANSFER_BIT,
	                                    VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, VK_ACCESS_SHADER_READ_BIT,
	                                    VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	command_buffer->End();

	std::shared_ptr<myvk::Fence> fence = myvk::Fence::Create(command_buffer->GetDevicePtr());
	command_buffer->Submit(fence);
	fence->Wait();
}

void GlobalTexture::create_descriptors(const std::shared_ptr<myvk::Device> &device) {
	m_descriptor_pool = myvk::DescriptorPool::Create(device, 1, {{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1}});
	{
		VkDescriptorSetLayoutBinding block_texture_binding = {};
		block_texture_binding.binding = 0;
		block_texture_binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		block_texture_binding.descriptorCount = 1;
		block_texture_binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

		m_descriptor_set_layout = myvk::DescriptorSetLayout::Create(device, {block_texture_binding});
	}

	m_descriptor_set = myvk::DescriptorSet::Create(m_descriptor_pool, m_descriptor_set_layout);
	m_descriptor_set->UpdateCombinedImageSampler(m_block_sampler, m_block_view, 0);
}
