#ifndef CUBECRAFT3_CLIENT_PASS_CHUNK_TRANSPARENT_PASS_HPP
#define CUBECRAFT3_CLIENT_PASS_CHUNK_TRANSPARENT_PASS_HPP

#include <client/ChunkMesh.hpp>

#include <myvk_rg/RenderGraph.hpp>

class ChunkTransparentPass final : public myvk_rg::GraphicsPassBase {
private:
	const std::vector<std::shared_ptr<ChunkMeshCluster>> *m_p_prepared_clusters;

	myvk::Ptr<myvk::GraphicsPipeline> m_pipeline;

public:
	inline void Initialize(myvk_rg::ImageInput block_texture_image, myvk_rg::ImageInput light_map_image,
	                       myvk_rg::BufferInput mesh_info_buffer, myvk_rg::BufferInput camera_buffer,
	                       myvk_rg::ImageInput depth_image, myvk_rg::BufferInput draw_cmd_buffer,
	                       myvk_rg::BufferInput draw_count_buffer) {
		AddDescriptorInput<0, myvk_rg::Usage::kStorageBufferR, VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT>({"mesh_info"},
		                                                                                              mesh_info_buffer);
		AddDescriptorInput<1, myvk_rg::Usage::kUniformBuffer, VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT>({"camera"},
		                                                                                             camera_buffer);
		AddDescriptorInput<2, myvk_rg::Usage::kSampledImage, VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT>(
		    {"block_texture"}, block_texture_image,
		    myvk::Sampler::Create(GetRenderGraphPtr()->GetDevicePtr(), VK_FILTER_NEAREST,
		                          VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_SAMPLER_MIPMAP_MODE_LINEAR));
		AddDescriptorInput<3, myvk_rg::Usage::kSampledImage, VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT>(
		    {"light_map"}, light_map_image,
		    myvk::Sampler::Create(GetRenderGraphPtr()->GetDevicePtr(), VK_FILTER_LINEAR,
		                          VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE));

		auto accum = CreateResource<myvk_rg::ManagedImage>({"accum"}, VK_FORMAT_R16G16B16A16_SFLOAT);
		accum->SetLoadOp(VK_ATTACHMENT_LOAD_OP_CLEAR);
		accum->SetClearColorValue({0.0f, 0.0f, 0.0f, 0.0f});

		auto reveal = CreateResource<myvk_rg::ManagedImage>({"reveal"}, VK_FORMAT_R8_UNORM);
		reveal->SetLoadOp(VK_ATTACHMENT_LOAD_OP_CLEAR);
		reveal->SetClearColorValue({1.0f});

		AddColorAttachmentInput<0, myvk_rg::Usage::kColorAttachmentRW>({"accum"}, accum);
		AddColorAttachmentInput<1, myvk_rg::Usage::kColorAttachmentRW>({"reveal"}, reveal);
		SetDepthAttachmentInput<myvk_rg::Usage::kDepthAttachmentR>({"depth"}, depth_image);

		AddInput<myvk_rg::Usage::kDrawIndirectBuffer>({"draw_cmd"}, draw_cmd_buffer);
		AddInput<myvk_rg::Usage::kDrawIndirectBuffer>({"draw_count"}, draw_count_buffer);
	}
	inline void Update(const std::vector<std::shared_ptr<ChunkMeshCluster>> &prepared_clusters) {
		m_p_prepared_clusters = &prepared_clusters;
	}

	inline void CreatePipeline() final {
		auto pipeline_layout =
		    myvk::PipelineLayout::Create(GetRenderGraphPtr()->GetDevicePtr(), {GetVkDescriptorSetLayout()},
		                                 {{VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(uint32_t)}});

		const auto &device = GetRenderGraphPtr()->GetDevicePtr();

		constexpr uint32_t kChunkVertSpv[] = {
#include <client/shader/chunk.vert.u32>
		};
		constexpr uint32_t kChunkTransparentFragSpv[] = {
#include <client/shader/chunk_transparent.frag.u32>
		};

		std::shared_ptr<myvk::ShaderModule> vert_shader_module, frag_shader_module;
		vert_shader_module = myvk::ShaderModule::Create(device, kChunkVertSpv, sizeof(kChunkVertSpv));
		frag_shader_module =
		    myvk::ShaderModule::Create(device, kChunkTransparentFragSpv, sizeof(kChunkTransparentFragSpv));

		std::vector<VkPipelineShaderStageCreateInfo> shader_stages = {
		    vert_shader_module->GetPipelineShaderStageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT),
		    frag_shader_module->GetPipelineShaderStageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT)};

		myvk::GraphicsPipelineState pipeline_state = {};
		pipeline_state.m_vertex_input_state.Enable({{0, sizeof(ChunkMeshVertex), VK_VERTEX_INPUT_RATE_VERTEX}},
		                                           {{0, 0, VK_FORMAT_R32G32_UINT, 0}});
		pipeline_state.m_input_assembly_state.Enable(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
		pipeline_state.m_rasterization_state.Initialize(VK_POLYGON_MODE_FILL, VK_FRONT_FACE_COUNTER_CLOCKWISE,
		                                                VK_CULL_MODE_NONE);
		pipeline_state.m_depth_stencil_state.Enable(VK_TRUE, VK_FALSE);
		pipeline_state.m_multisample_state.Enable(VK_SAMPLE_COUNT_1_BIT);

		VkExtent2D extent = GetRenderGraphPtr()->GetCanvasSize();
		pipeline_state.m_viewport_state.Enable({{0, 0, (float)extent.width, (float)extent.height, 0.0f, 1.0f}},
		                                       {{{0, 0}, extent}});

		// WBOIT blend functions
		VkPipelineColorBlendAttachmentState accum = {};
		accum.blendEnable = VK_TRUE;
		accum.srcColorBlendFactor = accum.srcAlphaBlendFactor = accum.dstColorBlendFactor = accum.dstAlphaBlendFactor =
		    VK_BLEND_FACTOR_ONE;
		accum.colorBlendOp = accum.alphaBlendOp = VK_BLEND_OP_ADD;
		accum.colorWriteMask =
		    VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		VkPipelineColorBlendAttachmentState reveal = {};
		reveal.blendEnable = VK_TRUE;
		reveal.srcColorBlendFactor = VK_BLEND_FACTOR_ZERO;
		reveal.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;
		reveal.colorBlendOp = VK_BLEND_OP_ADD;
		reveal.colorWriteMask = VK_COLOR_COMPONENT_R_BIT;
		pipeline_state.m_color_blend_state.Enable({accum, reveal});

		m_pipeline = myvk::GraphicsPipeline::Create(pipeline_layout, GetVkRenderPass(), shader_stages, pipeline_state,
		                                            GetSubpass());
	}
	inline void CmdExecute(const myvk::Ptr<myvk::CommandBuffer> &command_buffer) const final {
		command_buffer->CmdBindPipeline(m_pipeline);
		command_buffer->CmdBindDescriptorSets({GetVkDescriptorSet()}, m_pipeline);

		const auto &draw_cmd_buffer = GetInput({"draw_cmd"})->GetResource<myvk_rg::BufferBase>()->GetVkBuffer();
		const auto &draw_count_buffer = GetInput({"draw_count"})->GetResource<myvk_rg::BufferBase>()->GetVkBuffer();

		for (const auto &cluster : *m_p_prepared_clusters) {
			command_buffer->CmdBindVertexBuffer(cluster->GetVertexBuffer(), 0);
			command_buffer->CmdBindIndexBuffer(cluster->GetIndexBuffer(), 0, cluster->kIndexType);
			uint32_t pc_data[] = {cluster->GetClusterOffset() * cluster->GetMaxMeshes()};
			command_buffer->CmdPushConstants(m_pipeline->GetPipelineLayoutPtr(), VK_SHADER_STAGE_VERTEX_BIT, 0,
			                                 sizeof(uint32_t), pc_data);
			command_buffer->CmdDrawIndexedIndirectCount(
			    draw_cmd_buffer, pc_data[0] * sizeof(VkDrawIndexedIndirectCommand), //
			    draw_count_buffer, cluster->GetClusterOffset() * sizeof(uint32_t),  //
			    std::min(cluster->GetLocalMeshCount(), cluster->GetMaxMeshes()));
		}
	}
	inline auto GetAccumOutput() { return MakeImageOutput({"accum"}); }
	inline auto GetRevealOutput() { return MakeImageOutput({"reveal"}); }
};

#endif
