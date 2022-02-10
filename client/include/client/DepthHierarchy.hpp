#ifndef CUBECRAFT3_CLIENT_DEPTH_PYRAMID_HPP
#define CUBECRAFT3_CLIENT_DEPTH_PYRAMID_HPP

#include <client/Canvas.hpp>
#include <client/Config.hpp>

#include <myvk/Buffer.hpp>
#include <myvk/ComputePipeline.hpp>

class DepthHierarchy {
private:
	std::shared_ptr<Canvas> m_canvas_ptr;

	std::shared_ptr<myvk::PipelineLayout> m_build_pipeline_layout;
	std::shared_ptr<myvk::ComputePipeline> m_build_pipeline;

	std::shared_ptr<myvk::DescriptorPool> m_descriptor_pool;
	std::shared_ptr<myvk::DescriptorSetLayout> m_build_descriptor_set_layout, m_output_descriptor_set_layout;

	std::shared_ptr<myvk::Sampler> m_sampler;

	std::shared_ptr<myvk::CommandPool> m_command_pool;

	struct FrameData {
		std::shared_ptr<myvk::Image> m_image;
		std::shared_ptr<myvk::ImageView> m_image_view;
		std::vector<std::shared_ptr<myvk::ImageView>> m_lod_image_views;

		std::vector<std::shared_ptr<myvk::DescriptorSet>> m_build_descriptor_sets;
		std::shared_ptr<myvk::DescriptorSet> m_output_descriptor_set;

		// std::shared_ptr<myvk::Buffer> m_build_buffer; // tmp buffer for copying depth image
		// Use a tmp buffer has huge performance impact

		std::shared_ptr<myvk::CommandBuffer> m_command_buffer;
	};
	FrameData m_frame_data[kFrameCount];

	void create_images();
	void create_descriptor_pool();
	void create_build_descriptors();
	void create_output_descriptors();
	void create_build_pipeline();
	void create_build_command_buffers();
	// void create_build_buffers();

public:
	inline static std::shared_ptr<DepthHierarchy> Create(const std::shared_ptr<Canvas> &canvas_ptr) {
		std::shared_ptr<DepthHierarchy> ret = std::make_shared<DepthHierarchy>();
		ret->m_canvas_ptr = canvas_ptr;
		ret->Resize();
		return ret;
	}

	inline void Resize() {
		create_images();
		create_descriptor_pool();
		create_build_descriptors();
		// create_build_buffers();
		create_build_pipeline();
		create_build_command_buffers();
		create_output_descriptors();
	}

	void CmdBuild(const std::shared_ptr<myvk::CommandBuffer> &command_buffer) const;

	inline const std::shared_ptr<Canvas> &GetCanvasPtr() const { return m_canvas_ptr; }

	inline const std::shared_ptr<myvk::DescriptorSetLayout> &GetDescriptorSetLayout() const {
		return m_output_descriptor_set_layout;
	}
	inline const std::shared_ptr<myvk::DescriptorSet> &GetFrameDescriptorSet(uint32_t frame) const {
		return m_frame_data[frame].m_output_descriptor_set;
	}
};

#endif
