#ifndef HYPERCRAFT_CLIENT_PASS_DEPTHHIERARCHYPASS_HPP
#define HYPERCRAFT_CLIENT_PASS_DEPTHHIERARCHYPASS_HPP

#include <myvk_rg/RenderGraph.hpp>

namespace hc::client::rg {

class DepthHierarchyPass final : public myvk_rg::PassGroupBase {
private:
	myvk_rg::ImageInput m_depth_image{};
	uint32_t m_level_count = 0u;

	class DHSubpass final : public myvk_rg::GraphicsPassBase {
	private:
		myvk::Ptr<myvk::GraphicsPipeline> m_pipeline;
		uint32_t m_level = 0u;

	public:
		inline void Initialize(uint32_t cur_level, myvk_rg::ImageInput prev_image,
		                       const myvk::Ptr<myvk::Sampler> &sampler) {
			m_level = cur_level;
			auto cur_image = CreateResource<myvk_rg::ManagedImage>({"cur_image"}, VK_FORMAT_R32_SFLOAT);
			cur_image->SetSizeFunc([cur_level](const VkExtent2D &extent) {
				VkExtent2D half_extent = {std::max(extent.width >> 1u, 1u), std::max(extent.height >> 1u, 1u)};
				return myvk_rg::SubImageSize{half_extent, 1, cur_level - 1, 1};
			});
			AddDescriptorInput<0, myvk_rg::Usage::kSampledImage, VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT>(
			    {"prev_image"}, prev_image, sampler);
			AddColorAttachmentInput<0, myvk_rg::Usage::kColorAttachmentW>({"cur_image"}, cur_image);
		}

		inline void CreatePipeline() final {
			auto pipeline_layout =
			    myvk::PipelineLayout::Create(GetRenderGraphPtr()->GetDevicePtr(), {GetVkDescriptorSetLayout()}, {});

			const auto &device = GetRenderGraphPtr()->GetDevicePtr();

			constexpr uint32_t kQuadVertSpv[] = {
#include <client/shader/quad.vert.u32>
			};
			constexpr uint32_t kDHFragSpv[] = {
#include <client/shader/depth_hierarchy.frag.u32>
			};

			std::shared_ptr<myvk::ShaderModule> vert_shader_module, frag_shader_module;
			vert_shader_module = myvk::ShaderModule::Create(device, kQuadVertSpv, sizeof(kQuadVertSpv));
			frag_shader_module = myvk::ShaderModule::Create(device, kDHFragSpv, sizeof(kDHFragSpv));

			std::vector<VkPipelineShaderStageCreateInfo> shader_stages = {
			    vert_shader_module->GetPipelineShaderStageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT),
			    frag_shader_module->GetPipelineShaderStageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT)};

			myvk::GraphicsPipelineState pipeline_state = {};
			auto extent = GetRenderGraphPtr()->GetCanvasSize();
			extent.width >>= m_level;
			extent.height >>= m_level;
			extent.width = std::max(1u, extent.width);
			extent.height = std::max(1u, extent.height);
			pipeline_state.m_viewport_state.Enable(
			    std::vector<VkViewport>{{0, 0, (float)extent.width, (float)extent.height}},
			    std::vector<VkRect2D>{{{0, 0}, extent}});
			pipeline_state.m_vertex_input_state.Enable();
			pipeline_state.m_input_assembly_state.Enable(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
			pipeline_state.m_rasterization_state.Initialize(VK_POLYGON_MODE_FILL, VK_FRONT_FACE_COUNTER_CLOCKWISE,
			                                                VK_CULL_MODE_FRONT_BIT);
			pipeline_state.m_multisample_state.Enable(VK_SAMPLE_COUNT_1_BIT);
			pipeline_state.m_color_blend_state.Enable(1, VK_FALSE);

			m_pipeline = myvk::GraphicsPipeline::Create(pipeline_layout, GetVkRenderPass(), shader_stages,
			                                            pipeline_state, GetSubpass());
		}

		inline void CmdExecute(const myvk::Ptr<myvk::CommandBuffer> &command_buffer) const final {
			command_buffer->CmdBindPipeline(m_pipeline);
			command_buffer->CmdBindDescriptorSets({GetVkDescriptorSet()}, m_pipeline);
			command_buffer->CmdDraw(3, 1, 0, 0);
		}

		inline auto GetCurLevelOutput() { return MakeImageOutput({"cur_image"}); }
	};

	class DHSubpass2 final : public myvk_rg::GraphicsPassBase {
	private:
		myvk::Ptr<myvk::GraphicsPipeline> m_pipeline;
		uint32_t m_level = 0u;

	public:
		inline void Initialize(uint32_t cur_level, myvk_rg::ImageInput prev_image,
		                       const myvk::Ptr<myvk::Sampler> &sampler) {
			m_level = cur_level;
			auto cur_image = CreateResource<myvk_rg::ManagedImage>({"cur_image"}, VK_FORMAT_R32_SFLOAT);
			cur_image->SetSizeFunc([cur_level](const VkExtent2D &extent) {
				VkExtent2D half_extent = {std::max(extent.width >> 1u, 1u), std::max(extent.height >> 1u, 1u)};
				return myvk_rg::SubImageSize{half_extent, 1, cur_level - 1, 1};
			});
			auto next_image = CreateResource<myvk_rg::ManagedImage>({"next_image"}, VK_FORMAT_R32_SFLOAT);
			next_image->SetSizeFunc([cur_level](const VkExtent2D &extent) {
				VkExtent2D half_extent = {std::max(extent.width >> 1u, 1u), std::max(extent.height >> 1u, 1u)};
				return myvk_rg::SubImageSize{half_extent, 1, cur_level, 1};
			});
			AddDescriptorInput<0, myvk_rg::Usage::kSampledImage, VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT>(
			    {"prev_image"}, prev_image, sampler);
			AddDescriptorInput<1, myvk_rg::Usage::kStorageImageRW, VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT>(
			    {"next_image"}, next_image);
			AddColorAttachmentInput<0, myvk_rg::Usage::kColorAttachmentW>({"cur_image"}, cur_image);
		}

		inline void CreatePipeline() final {
			auto pipeline_layout =
			    myvk::PipelineLayout::Create(GetRenderGraphPtr()->GetDevicePtr(), {GetVkDescriptorSetLayout()}, {});

			const auto &device = GetRenderGraphPtr()->GetDevicePtr();

			constexpr uint32_t kQuadVertSpv[] = {
#include <client/shader/quad.vert.u32>
			};
			constexpr uint32_t kDHFragSpv[] = {
#include <client/shader/depth_hierarchy_2.frag.u32>
			};

			std::shared_ptr<myvk::ShaderModule> vert_shader_module, frag_shader_module;
			vert_shader_module = myvk::ShaderModule::Create(device, kQuadVertSpv, sizeof(kQuadVertSpv));
			frag_shader_module = myvk::ShaderModule::Create(device, kDHFragSpv, sizeof(kDHFragSpv));

			std::vector<VkPipelineShaderStageCreateInfo> shader_stages = {
			    vert_shader_module->GetPipelineShaderStageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT),
			    frag_shader_module->GetPipelineShaderStageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT)};

			myvk::GraphicsPipelineState pipeline_state = {};
			auto extent = GetRenderGraphPtr()->GetCanvasSize();
			extent.width >>= m_level;
			extent.height >>= m_level;
			extent.width = std::max(1u, extent.width);
			extent.height = std::max(1u, extent.height);
			pipeline_state.m_viewport_state.Enable(
			    std::vector<VkViewport>{{0, 0, (float)extent.width, (float)extent.height}},
			    std::vector<VkRect2D>{{{0, 0}, extent}});
			pipeline_state.m_vertex_input_state.Enable();
			pipeline_state.m_input_assembly_state.Enable(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
			pipeline_state.m_rasterization_state.Initialize(VK_POLYGON_MODE_FILL, VK_FRONT_FACE_COUNTER_CLOCKWISE,
			                                                VK_CULL_MODE_FRONT_BIT);
			pipeline_state.m_multisample_state.Enable(VK_SAMPLE_COUNT_1_BIT);
			pipeline_state.m_color_blend_state.Enable(1, VK_FALSE);

			m_pipeline = myvk::GraphicsPipeline::Create(pipeline_layout, GetVkRenderPass(), shader_stages,
			                                            pipeline_state, GetSubpass());
		}

		inline void CmdExecute(const myvk::Ptr<myvk::CommandBuffer> &command_buffer) const final {
			command_buffer->CmdBindPipeline(m_pipeline);
			command_buffer->CmdBindDescriptorSets({GetVkDescriptorSet()}, m_pipeline);
			command_buffer->CmdDraw(3, 1, 0, 0);
		}

		inline auto GetCurLevelOutput() { return MakeImageOutput({"cur_image"}); }
		inline auto GetNextLevelOutput() { return MakeImageOutput({"next_image"}); }
	};

public:
	inline void Initialize(myvk_rg::ImageInput depth_image) {
		m_depth_image = depth_image;
		UpdateLevelCount();
	}

	inline void UpdateLevelCount() {
		SetLevelCount(myvk::ImageBase::QueryMipLevel(GetRenderGraphPtr()->GetCanvasSize()));
	}
	inline void SetLevelCount(uint32_t level_count) {
		level_count = std::max(level_count, 2u);
		if (m_level_count == level_count)
			return;
		m_level_count = level_count;

		auto sampler = myvk::Sampler::Create(GetRenderGraphPtr()->GetDevicePtr(), VK_FILTER_NEAREST,
		                                     VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);

		ClearPasses();
		std::vector<myvk_rg::ImageOutput> outputs(m_level_count - 1);

		auto extent = GetRenderGraphPtr()->GetCanvasSize();
		for (uint32_t i = 1; i < level_count;) {
			extent = {std::max(extent.width >> 1u, 1u), std::max(extent.height >> 1u, 1u)};
			if (i + 1 < level_count && (extent.width & 1u) == 0u && (extent.height & 1u) == 0u) {
				extent.width >>= 1u, extent.height >>= 1u;

				auto subpass = CreatePass<DHSubpass2>({"subpass", i}, i,                       //
				                                      i == 1 ? m_depth_image : outputs[i - 2], //
				                                      sampler);
				outputs[i - 1] = subpass->GetCurLevelOutput();
				outputs[i] = subpass->GetNextLevelOutput();

				i += 2;
			} else {
				auto subpass = CreatePass<DHSubpass>({"subpass", i}, i,                       //
				                                     i == 1 ? m_depth_image : outputs[i - 2], //
				                                     sampler);
				outputs[i - 1] = subpass->GetCurLevelOutput();

				++i;
			}
		}
		CreateResourceForce<myvk_rg::CombinedImage>({"depth_hierarchy"}, VK_IMAGE_VIEW_TYPE_2D, std::move(outputs));
	}
	inline auto GetDepthHierarchyOutput() { return GetResource<myvk_rg::CombinedImage>({"depth_hierarchy"}); }
};

} // namespace hc::client::rg

#endif
