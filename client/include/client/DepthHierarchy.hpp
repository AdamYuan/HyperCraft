#ifndef CUBECRAFT3_CLIENT_DEPTH_PYRAMID_HPP
#define CUBECRAFT3_CLIENT_DEPTH_PYRAMID_HPP

#include <client/Config.hpp>

#include <myvk/Buffer.hpp>
#include <myvk/ComputePipeline.hpp>
#include <myvk/FrameManager.hpp>
#include <myvk/Image.hpp>

class DepthHierarchy {
private:
	std::shared_ptr<myvk::FrameManager> m_frame_manager_ptr;

	std::shared_ptr<myvk::PipelineLayout> m_build_pipeline_layout;
	std::shared_ptr<myvk::ComputePipeline> m_build_pipeline;

	std::shared_ptr<myvk::DescriptorPool> m_descriptor_pool;
	std::shared_ptr<myvk::DescriptorSetLayout> m_build_descriptor_set_layout, m_output_descriptor_set_layout;

	std::shared_ptr<myvk::Sampler> m_sampler;

	struct FrameData {
		std::shared_ptr<myvk::Image> m_image;
		std::shared_ptr<myvk::ImageView> m_image_view;
		std::vector<std::shared_ptr<myvk::ImageView>> m_lod_image_views;

		std::vector<std::shared_ptr<myvk::DescriptorSet>> m_build_descriptor_sets;
		std::shared_ptr<myvk::DescriptorSet> m_output_descriptor_set;
	};
	FrameData m_frame_data[kFrameCount];

	void create_images();
	void create_descriptor_pool();
	void create_build_descriptors();
	void create_output_descriptors();
	void create_build_pipeline();
	// void create_build_buffers();

public:
	inline static std::shared_ptr<DepthHierarchy> Create(const std::shared_ptr<myvk::FrameManager> &frame_manager_ptr) {
		std::shared_ptr<DepthHierarchy> ret = std::make_shared<DepthHierarchy>();
		ret->m_frame_manager_ptr = frame_manager_ptr;
		ret->Resize();
		return ret;
	}

	inline void Resize() {
		create_images();
		create_descriptor_pool();
		create_build_descriptors();
		// create_build_buffers();
		create_build_pipeline();
		create_output_descriptors();
	}

	void CmdBuild(const std::shared_ptr<myvk::CommandBuffer> &command_buffer) const;

	inline const std::shared_ptr<myvk::DescriptorSetLayout> &GetDescriptorSetLayout() const {
		return m_output_descriptor_set_layout;
	}
	inline const std::shared_ptr<myvk::DescriptorSet> &GetFrameDescriptorSet(uint32_t frame) const {
		return m_frame_data[frame].m_output_descriptor_set;
	}

	inline const std::shared_ptr<myvk::ImageView> &GetAttachmentImageView() const {
		return m_frame_data[m_frame_manager_ptr->GetCurrentFrame()].m_lod_image_views[0];
	}
};

#endif
