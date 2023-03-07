#ifndef CUBECRAFT3_CLIENT_GLOBAL_TEXTURE_HPP
#define CUBECRAFT3_CLIENT_GLOBAL_TEXTURE_HPP

#include <myvk/CommandPool.hpp>
#include <myvk/DescriptorSet.hpp>
#include <myvk/Image.hpp>
#include <myvk/ImageView.hpp>
#include <myvk/Sampler.hpp>

class GlobalTexture {
private:
	std::shared_ptr<myvk::Image> m_block_texture;
	std::shared_ptr<myvk::ImageView> m_block_view;

	std::shared_ptr<myvk::Image> m_lightmap_texture;
	std::shared_ptr<myvk::ImageView> m_lightmap_view;

	void create_block_texture(const std::shared_ptr<myvk::CommandPool> &command_pool);
	void create_lightmap_texture(const std::shared_ptr<myvk::CommandPool> &command_pool);

public:
	static std::shared_ptr<GlobalTexture> Create(const std::shared_ptr<myvk::CommandPool> &command_pool);

	inline const auto &GetBlockTextureView() const { return m_block_view; }
	inline const auto &GetLightMapView() const { return m_lightmap_view; }
};

#endif
