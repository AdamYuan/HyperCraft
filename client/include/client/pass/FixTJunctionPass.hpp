#ifndef CUBECRAFT3_CLIENT_PASS_POSTPROCESSPASS_HPP
#define CUBECRAFT3_CLIENT_PASS_POSTPROCESSPASS_HPP

#include <myvk_rg/RenderGraph.hpp>

class FixTJunctionPass final : public myvk_rg::GraphicsPassBase {
private:
	myvk::Ptr<myvk::GraphicsPipeline> m_pipeline;

public:
	inline void Initialize(myvk_rg::ImageInput color_image, myvk_rg::ImageInput depth_image,
	                       myvk_rg::ImageInput fixed_color_image) {
		auto sampler = myvk::Sampler::Create(GetRenderGraphPtr()->GetDevicePtr(), VK_FILTER_NEAREST,
		                                     VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);
		auto fixed_depth_image = CreateResource<myvk_rg::ManagedImage>({"fixed_depth"}, VK_FORMAT_R32_SFLOAT);
		AddDescriptorInput<0, myvk_rg::Usage::kSampledImage, VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT>(
		    {"color"}, color_image, sampler);
		AddDescriptorInput<1, myvk_rg::Usage::kSampledImage, VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT>(
		    {"depth"}, depth_image, sampler);
		AddColorAttachmentInput<0, myvk_rg::Usage::kColorAttachmentW>({"fixed_color"}, fixed_color_image);
		AddColorAttachmentInput<1, myvk_rg::Usage::kColorAttachmentW>({"fixed_depth"}, fixed_depth_image);
	}
	inline void Initialize(myvk_rg::ImageInput color_image, myvk_rg::ImageInput depth_image) {
		auto fixed_color_image = CreateResource<myvk_rg::ManagedImage>({"fixed_color"}, color_image->GetFormat());
		Initialize(color_image, depth_image, fixed_color_image);
	}

	inline void CreatePipeline() final {
		auto pipeline_layout =
		    myvk::PipelineLayout::Create(GetRenderGraphPtr()->GetDevicePtr(), {GetVkDescriptorSetLayout()}, {});

		const auto &device = GetRenderGraphPtr()->GetDevicePtr();

		constexpr uint32_t kQuadVertSpv[] = {
#include <client/shader/quad.vert.u32>
		};
		constexpr uint32_t kFixTJunctionFragSpv[] = {
#include <client/shader/post_process.frag.u32>
		};

		std::shared_ptr<myvk::ShaderModule> vert_shader_module, frag_shader_module;
		vert_shader_module = myvk::ShaderModule::Create(device, kQuadVertSpv, sizeof(kQuadVertSpv));
		frag_shader_module = myvk::ShaderModule::Create(device, kFixTJunctionFragSpv, sizeof(kFixTJunctionFragSpv));

		std::vector<VkPipelineShaderStageCreateInfo> shader_stages = {
		    vert_shader_module->GetPipelineShaderStageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT),
		    frag_shader_module->GetPipelineShaderStageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT)};

		myvk::GraphicsPipelineState pipeline_state = {};
		auto extent = GetRenderGraphPtr()->GetCanvasSize();
		pipeline_state.m_viewport_state.Enable(
		    std::vector<VkViewport>{{0, 0, (float)extent.width, (float)extent.height}},
		    std::vector<VkRect2D>{{{0, 0}, extent}});
		pipeline_state.m_vertex_input_state.Enable();
		pipeline_state.m_input_assembly_state.Enable(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
		pipeline_state.m_rasterization_state.Initialize(VK_POLYGON_MODE_FILL, VK_FRONT_FACE_COUNTER_CLOCKWISE,
		                                                VK_CULL_MODE_FRONT_BIT);
		pipeline_state.m_multisample_state.Enable(VK_SAMPLE_COUNT_1_BIT);
		pipeline_state.m_color_blend_state.Enable(2, VK_FALSE);

		m_pipeline = myvk::GraphicsPipeline::Create(pipeline_layout, GetVkRenderPass(), shader_stages, pipeline_state,
		                                            GetSubpass());
	}

	inline void CmdExecute(const myvk::Ptr<myvk::CommandBuffer> &command_buffer) const final {
		command_buffer->CmdBindPipeline(m_pipeline);
		command_buffer->CmdBindDescriptorSets({GetVkDescriptorSet()}, m_pipeline);
		command_buffer->CmdDraw(3, 1, 0, 0);
	}

	inline auto GetFixedColorOutput() { return MakeImageOutput({"fixed_color"}); }
	inline auto GetFixedDepthOutput() { return MakeImageOutput({"fixed_depth"}); }
};

#endif
