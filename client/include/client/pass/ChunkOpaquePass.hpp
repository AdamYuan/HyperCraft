#ifndef CUBECRAFT3_CLIENT_PASS_CHUNKOPAQUEPASS_HPP
#define CUBECRAFT3_CLIENT_PASS_CHUNKOPAQUEPASS_HPP

#include <client/Camera.hpp>
#include <client/ChunkMesh.hpp>
#include <client/GlobalTexture.hpp>

#include <myvk_rg/RenderGraph.hpp>

class ChunkOpaquePass final : public myvk_rg::GraphicsPassBase {
private:
	std::shared_ptr<ChunkMeshPool> m_chunk_mesh_pool_ptr;
	std::shared_ptr<GlobalTexture> m_global_texture_ptr;

	const std::vector<std::shared_ptr<ChunkMeshCluster>> *m_p_prepared_clusters;

	myvk::Ptr<myvk::GraphicsPipeline> m_pipeline;

public:
	MYVK_RG_INLINE_INITIALIZER(const std::shared_ptr<ChunkMeshPool> &chunk_mesh_pool_ptr,
	                           const std::shared_ptr<GlobalTexture> &global_texture_ptr,
	                           myvk_rg::BufferInput camera_buffer, //
	                           myvk_rg::ImageInput color_image, myvk_rg::ImageInput depth_image,
	                           myvk_rg::BufferInput draw_cmd_buffer, myvk_rg::BufferInput draw_count_buffer) {
		m_chunk_mesh_pool_ptr = chunk_mesh_pool_ptr;
		m_global_texture_ptr = global_texture_ptr;

		AddDescriptorInput<0, myvk_rg::Usage::kUniformBuffer, VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT>({"camera"},
		                                                                                             camera_buffer);
		AddColorAttachmentInput<0, myvk_rg::Usage::kColorAttachmentW>({"opaque"}, color_image);
		SetDepthAttachmentInput<myvk_rg::Usage::kDepthAttachmentRW>({"depth"}, depth_image);
		AddInput<myvk_rg::Usage::kDrawIndirectBuffer>({"draw_cmd"}, draw_cmd_buffer);
		AddInput<myvk_rg::Usage::kDrawIndirectBuffer>({"draw_count"}, draw_count_buffer);
	}
	inline void Update(const std::vector<std::shared_ptr<ChunkMeshCluster>> &prepared_clusters) {
		m_p_prepared_clusters = &prepared_clusters;
	}

	inline void CreatePipeline() final {
		auto pipeline_layout = myvk::PipelineLayout::Create(GetRenderGraphPtr()->GetDevicePtr(),
		                                                    {
		                                                        GetVkDescriptorSetLayout(),
		                                                        m_global_texture_ptr->GetDescriptorSetLayout(),
		                                                        m_chunk_mesh_pool_ptr->GetDescriptorSetLayout(),
		                                                    },
		                                                    {});

		const auto &device = GetRenderGraphPtr()->GetDevicePtr();

		constexpr uint32_t kChunkVertSpv[] = {
#include <client/shader/chunk.vert.u32>
		};
		constexpr uint32_t kChunkOpaqueFragSpv[] = {
#include <client/shader/chunk_opaque.frag.u32>
		};

		std::shared_ptr<myvk::ShaderModule> vert_shader_module, frag_shader_module;
		vert_shader_module = myvk::ShaderModule::Create(device, kChunkVertSpv, sizeof(kChunkVertSpv));
		frag_shader_module = myvk::ShaderModule::Create(device, kChunkOpaqueFragSpv, sizeof(kChunkOpaqueFragSpv));

		std::vector<VkPipelineShaderStageCreateInfo> shader_stages = {
		    vert_shader_module->GetPipelineShaderStageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT),
		    frag_shader_module->GetPipelineShaderStageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT)};

		myvk::GraphicsPipelineState pipeline_state = {};
		pipeline_state.m_vertex_input_state.Enable({{0, sizeof(ChunkMeshVertex), VK_VERTEX_INPUT_RATE_VERTEX}},
		                                           {{0, 0, VK_FORMAT_R32G32_UINT, 0}});
		pipeline_state.m_input_assembly_state.Enable(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
		pipeline_state.m_rasterization_state.Initialize(VK_POLYGON_MODE_FILL, VK_FRONT_FACE_COUNTER_CLOCKWISE,
		                                                VK_CULL_MODE_BACK_BIT);
		pipeline_state.m_depth_stencil_state.Enable(VK_TRUE, VK_TRUE);
		pipeline_state.m_multisample_state.Enable(VK_SAMPLE_COUNT_1_BIT);
		pipeline_state.m_color_blend_state.Enable(1, VK_FALSE);
		auto extent = GetRenderGraphPtr()->GetCanvasSize();
		pipeline_state.m_viewport_state.Enable(
		    std::vector<VkViewport>{{0, 0, (float)extent.width, (float)extent.height, 0.0f, 1.0f}},
		    std::vector<VkRect2D>{{{0, 0}, extent}});

		m_pipeline = myvk::GraphicsPipeline::Create(pipeline_layout, GetVkRenderPass(), shader_stages, pipeline_state,
		                                            GetSubpass());
	}
	inline auto GetOpaqueOutput() { return MakeImageOutput({"opaque"}); }
	inline auto GetDepthOutput() { return MakeImageOutput({"depth"}); }
	inline void CmdExecute(const myvk::Ptr<myvk::CommandBuffer> &command_buffer) const final {
		command_buffer->CmdBindPipeline(m_pipeline);
		command_buffer->CmdBindDescriptorSets({GetVkDescriptorSet(), m_global_texture_ptr->GetDescriptorSet()},
		                                      m_pipeline);

		const auto &draw_cmd_buffer = GetInput({"draw_cmd"})->GetResource<myvk_rg::BufferBase>()->GetVkBuffer();
		const auto &draw_count_buffer = GetInput({"draw_count"})->GetResource<myvk_rg::BufferBase>()->GetVkBuffer();

		uint32_t id = 0,
		         draw_cmd_size = m_chunk_mesh_pool_ptr->GetMaxMeshesPerCluster() * sizeof(VkDrawIndexedIndirectCommand);
		for (const auto &cluster : *m_p_prepared_clusters) {
			const auto &mesh_info_desc_set = cluster->GetMeshInfoDescriptorSet();
			command_buffer->CmdBindDescriptorSets({mesh_info_desc_set}, 2, m_pipeline);

			const auto &vertex_buffer = cluster->GetVertexBuffer();
			const auto &index_buffer = cluster->GetIndexBuffer();
			command_buffer->CmdBindVertexBuffer(vertex_buffer, 0);
			command_buffer->CmdBindIndexBuffer(index_buffer, 0, cluster->kIndexType);
			command_buffer->CmdDrawIndexedIndirectCount(
			    draw_cmd_buffer, id * draw_cmd_size,      //
			    draw_count_buffer, id * sizeof(uint32_t), //
			    std::min(cluster->GetLocalMeshCount(), cluster->GetMaxMeshes()));
			++id;
		}
	}
};

#endif
