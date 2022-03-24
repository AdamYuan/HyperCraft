#ifndef CUBECRAFT3_CLIENT_GLOBAL_TEXTURE_HPP
#define CUBECRAFT3_CLIENT_GLOBAL_TEXTURE_HPP

#include <myvk/CommandPool.hpp>
#include <myvk/DescriptorSet.hpp>
#include <myvk/Image.hpp>
#include <myvk/ImageView.hpp>
#include <myvk/Sampler.hpp>

class GlobalTexture {
private:
	std::shared_ptr<myvk::DescriptorPool> m_descriptor_pool;
	std::shared_ptr<myvk::DescriptorSetLayout> m_descriptor_set_layout;
	std::shared_ptr<myvk::DescriptorSet> m_descriptor_set;

	std::shared_ptr<myvk::Image> m_block_texture;
	std::shared_ptr<myvk::ImageView> m_block_view;
	std::shared_ptr<myvk::Sampler> m_block_sampler;

	std::shared_ptr<myvk::Image> m_lightmap_texture;
	std::shared_ptr<myvk::ImageView> m_lightmap_view;
	std::shared_ptr<myvk::Sampler> m_lightmap_sampler;

	void create_block_texture(const std::shared_ptr<myvk::CommandPool> &command_pool);
	void create_lightmap_texture(const std::shared_ptr<myvk::CommandPool> &command_pool);
	void create_descriptors(const std::shared_ptr<myvk::Device> &device);

public:
	static std::shared_ptr<GlobalTexture> Create(const std::shared_ptr<myvk::CommandPool> &command_pool);

	inline const std::shared_ptr<myvk::DescriptorSetLayout> &GetDescriptorSetLayout() const {
		return m_descriptor_set_layout;
	};
	inline const std::shared_ptr<myvk::DescriptorSet> &GetDescriptorSet() const { return m_descriptor_set; };
};

#endif
