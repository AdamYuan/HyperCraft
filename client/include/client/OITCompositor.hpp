#ifndef CUBECRAFT3_OIT_COMPOSITOR_HPP
#define CUBECRAFT3_OIT_COMPOSITOR_HPP

#include <client/Config.hpp>
#include <myvk/CommandBuffer.hpp>
#include <myvk/DescriptorSet.hpp>
#include <myvk/DescriptorSetLayout.hpp>
#include <myvk/GraphicsPipeline.hpp>
#include <myvk/RenderPass.hpp>

class OITCompositor {
private:
	std::shared_ptr<myvk::RenderPass> m_render_pass_ptr;

	std::shared_ptr<myvk::DescriptorPool> m_descriptor_pool;
	std::shared_ptr<myvk::DescriptorSetLayout> m_descriptor_set_layout;
	std::shared_ptr<myvk::DescriptorSet> m_descriptor_sets[kFrameCount];

	std::shared_ptr<myvk::ImageView> m_opaque_views[kFrameCount], m_accum_views[kFrameCount],
	    m_reveal_views[kFrameCount];

	std::shared_ptr<myvk::PipelineLayout> m_pipeline_layout;
	std::shared_ptr<myvk::GraphicsPipeline> m_pipeline;

	void create_descriptor();
	void create_pipeline(uint32_t subpass);

public:
	explicit OITCompositor(const std::shared_ptr<myvk::RenderPass> &render_pass_ptr, uint32_t subpass)
	    : m_render_pass_ptr{render_pass_ptr} {
		create_descriptor();
		create_pipeline(subpass);
	}
	inline static std::unique_ptr<OITCompositor> Create(const std::shared_ptr<myvk::RenderPass> &render_pass_ptr,
	                                                    uint32_t subpass) {
		return std::make_unique<OITCompositor>(render_pass_ptr, subpass);
	}
	void Update(uint32_t frame, const std::shared_ptr<myvk::ImageView> &opaque_view,
	            const std::shared_ptr<myvk::ImageView> &accum_view,
	            const std::shared_ptr<myvk::ImageView> &reveal_view);

	void CmdDrawPipeline(const std::shared_ptr<myvk::CommandBuffer> &command_buffer, uint32_t frame) const;
};

#endif
