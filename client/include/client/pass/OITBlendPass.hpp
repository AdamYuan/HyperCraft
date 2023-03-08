#ifndef CUBECRAFT3_CLIENT_PASS_OITBLENDPASS_HPP
#define CUBECRAFT3_CLIENT_PASS_OITBLENDPASS_HPP

#include <myvk_rg/RenderGraph.hpp>

class OITBlendPass final : public myvk_rg::GraphicsPassBase {
private:
	myvk::Ptr<myvk::GraphicsPipeline> m_pipeline;

public:
	MYVK_RG_INLINE_INITIALIZER(myvk_rg::ImageInput color_image, myvk_rg::ImageInput accum_image,
	                           myvk_rg::ImageInput reveal_image) {
		AddColorAttachmentInput<0, myvk_rg::Usage::kColorAttachmentRW>({"color"}, color_image);
		AddInputAttachmentInput<0, 0>({"accum"}, accum_image);
		AddInputAttachmentInput<1, 1>({"reveal"}, reveal_image);
	}

	inline void CreatePipeline() final {
		auto pipeline_layout =
		    myvk::PipelineLayout::Create(GetRenderGraphPtr()->GetDevicePtr(), {GetVkDescriptorSetLayout()}, {});

		const auto &device = GetRenderGraphPtr()->GetDevicePtr();

		constexpr uint32_t kQuadVertSpv[] = {
#include <client/shader/quad.vert.u32>
		};
		constexpr uint32_t kOITBlendFracSpv[] = {
#include <client/shader/oit_compositor.frag.u32>
		};

		std::shared_ptr<myvk::ShaderModule> vert_shader_module, frag_shader_module;
		vert_shader_module = myvk::ShaderModule::Create(device, kQuadVertSpv, sizeof(kQuadVertSpv));
		frag_shader_module = myvk::ShaderModule::Create(device, kOITBlendFracSpv, sizeof(kOITBlendFracSpv));

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

		// WBOIT blend functions
		VkPipelineColorBlendAttachmentState blend = {};
		blend.blendEnable = VK_TRUE;
		blend.srcColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		blend.dstColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
		blend.colorBlendOp = VK_BLEND_OP_ADD;

		blend.colorWriteMask =
		    VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT;
		pipeline_state.m_color_blend_state.Enable({blend});

		m_pipeline = myvk::GraphicsPipeline::Create(pipeline_layout, GetVkRenderPass(), shader_stages, pipeline_state,
		                                            GetSubpass());
	}

	inline void CmdExecute(const myvk::Ptr<myvk::CommandBuffer> &command_buffer) const final {
		command_buffer->CmdBindPipeline(m_pipeline);
		command_buffer->CmdBindDescriptorSets({GetVkDescriptorSet()}, m_pipeline);
		command_buffer->CmdDraw(3, 1, 0, 0);
	}

	inline auto GetColorOutput() { return MakeImageOutput({"color"}); }
};

#endif
