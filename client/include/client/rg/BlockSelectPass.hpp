#pragma once

#include <myvk_rg/RenderGraph.hpp>

#include <block/Block.hpp>
#include <block/BlockMesh.hpp>
#include <common/Position.hpp>

namespace hc::client::rg {

class BlockSelectPass final : public myvk_rg::GraphicsPassBase {
private:
	myvk::Ptr<myvk::GraphicsPipeline> m_pipeline;

	uint32_t m_aabb_count = 0;
	// Push Constants
	struct PCData {
		glm::i32vec3 m_position_aabb_cnt;
		uint32_t m_aabbs[block::kBlockMeshMaxAABBCount];
		static_assert(block::kBlockMeshMaxAABBCount == 4);
	} m_pc_data = {};

public:
	inline void Initialize(myvk_rg::BufferInput camera_buffer, myvk_rg::ImageInput color_image,
	                       myvk_rg::ImageInput depth_image) {
		AddDescriptorInput<0, myvk_rg::Usage::kUniformBuffer,
		                   VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_2_GEOMETRY_SHADER_BIT>(
		    {"camera"}, camera_buffer);
		AddColorAttachmentInput<0, myvk_rg::Usage::kColorAttachmentRW>({"color"}, color_image);
		SetDepthAttachmentInput<myvk_rg::Usage::kDepthAttachmentR>({"depth"}, depth_image);
	}

	inline void SetBlockSelection(const BlockPos3 &pos, block::Block block) {
		m_aabb_count = block.GetAABBCount();

		m_pc_data.m_position_aabb_cnt.x = pos.x;
		m_pc_data.m_position_aabb_cnt.y = pos.y;
		m_pc_data.m_position_aabb_cnt.z = pos.z;
		for (uint32_t i = 0; i < block.GetAABBCount(); ++i) {
			const auto &aabb = block.GetAABBs()[i];
			const auto &aabb_min = aabb.GetMin();
			const auto &aabb_max = aabb.GetMax();
			m_pc_data.m_aabbs[i] = aabb_min.x | (aabb_min.y << 5u) | (aabb_min.z << 10u) |
			                       ((aabb_max.x | (aabb_max.y << 5u) | (aabb_max.z << 10u)) << 16u);
		}
	}
	inline void UnsetBlockSelection() { m_aabb_count = 0; }

	inline void CreatePipeline() final {
		auto pipeline_layout =
		    myvk::PipelineLayout::Create(GetRenderGraphPtr()->GetDevicePtr(), {GetVkDescriptorSetLayout()},
		                                 {{VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(PCData)}});

		const auto &device = GetRenderGraphPtr()->GetDevicePtr();

		constexpr uint32_t kVertSpv[] = {
#include <client/shader/select.vert.u32>
		};
		constexpr uint32_t kGeomSpv[] = {
#include <client/shader/select.geom.u32>
		};
		constexpr uint32_t kFragSpv[] = {
#include <client/shader/select.frag.u32>
		};

		auto vert_shader_module = myvk::ShaderModule::Create(device, kVertSpv, sizeof(kVertSpv));
		auto geom_shader_module = myvk::ShaderModule::Create(device, kGeomSpv, sizeof(kGeomSpv));
		auto frag_shader_module = myvk::ShaderModule::Create(device, kFragSpv, sizeof(kFragSpv));

		std::vector<VkPipelineShaderStageCreateInfo> shader_stages = {
		    vert_shader_module->GetPipelineShaderStageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT),
		    geom_shader_module->GetPipelineShaderStageCreateInfo(VK_SHADER_STAGE_GEOMETRY_BIT),
		    frag_shader_module->GetPipelineShaderStageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT)};

		myvk::GraphicsPipelineState pipeline_state = {};
		auto extent = GetRenderGraphPtr()->GetCanvasSize();
		pipeline_state.m_viewport_state.Enable(
		    std::vector<VkViewport>{{0, 0, (float)extent.width, (float)extent.height, 0.0f, 1.0f}},
		    std::vector<VkRect2D>{{{0, 0}, extent}});
		pipeline_state.m_vertex_input_state.Enable();
		pipeline_state.m_input_assembly_state.Enable(VK_PRIMITIVE_TOPOLOGY_POINT_LIST);
		pipeline_state.m_rasterization_state.Initialize(VK_POLYGON_MODE_FILL, VK_FRONT_FACE_COUNTER_CLOCKWISE,
		                                                VK_CULL_MODE_FRONT_BIT);
		pipeline_state.m_rasterization_state.m_create_info.lineWidth = 1.5f;
		pipeline_state.m_depth_stencil_state.Enable(VK_TRUE, VK_FALSE);
		pipeline_state.m_multisample_state.Enable(VK_SAMPLE_COUNT_1_BIT);
		pipeline_state.m_color_blend_state.Enable(1, VK_FALSE);

		m_pipeline = myvk::GraphicsPipeline::Create(pipeline_layout, GetVkRenderPass(), shader_stages, pipeline_state,
		                                            GetSubpass());
	}

	inline void CmdExecute(const myvk::Ptr<myvk::CommandBuffer> &command_buffer) const final {
		command_buffer->CmdBindPipeline(m_pipeline);
		command_buffer->CmdBindDescriptorSets({GetVkDescriptorSet()}, m_pipeline);
		command_buffer->CmdPushConstants(m_pipeline->GetPipelineLayoutPtr(), VK_SHADER_STAGE_VERTEX_BIT, 0,
		                                 sizeof(PCData), &m_pc_data);
		command_buffer->CmdDraw(m_aabb_count, 1, 0, 0);
	}

	inline auto GetColorOutput() { return MakeImageOutput({"color"}); }
};

} // namespace hc::client::rg