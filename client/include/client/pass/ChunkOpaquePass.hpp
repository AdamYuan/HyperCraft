#ifndef CUBECRAFT3_CLIENT_PASS_CHUNKOPAQUEPASS_HPP
#define CUBECRAFT3_CLIENT_PASS_CHUNKOPAQUEPASS_HPP

#include <client/Camera.hpp>
#include <client/ChunkMesh.hpp>
#include <client/GlobalTexture.hpp>

#include <myvk_rg/RenderGraph.hpp>

class ChunkOpaquePass final : public myvk_rg::GraphicsPassBase {
private:
	std::shared_ptr<ChunkMeshRendererBase> m_renderer_ptr;
	std::shared_ptr<Camera> m_camera_ptr;
	std::shared_ptr<GlobalTexture> m_global_texture_ptr;

	const std::vector<ChunkMeshRendererBase::PreparedCluster> *m_p_prepared_clusters;

	myvk::Ptr<myvk::GraphicsPipeline> m_pipeline;

public:
	MYVK_RG_INLINE_INITIALIZER(const std::shared_ptr<ChunkMeshRendererBase> &renderer_ptr,
	                           const std::shared_ptr<Camera> &camera_ptr,
	                           const std::shared_ptr<GlobalTexture> &global_texture_ptr,
	                           myvk_rg::ImageInput color_image, myvk_rg::ImageInput depth_image,
	                           myvk_rg::BufferInput draw_cmd_buffer, myvk_rg::BufferInput draw_count_buffer) {
		m_renderer_ptr = renderer_ptr;
		m_camera_ptr = camera_ptr;
		m_global_texture_ptr = global_texture_ptr;

		AddColorAttachmentInput<0, myvk_rg::Usage::kColorAttachmentW>({"opaque"}, color_image);
		SetDepthAttachmentInput<myvk_rg::Usage::kDepthAttachmentRW>({"depth"}, depth_image);
		AddInput<myvk_rg::Usage::kDrawIndirectBuffer>({"draw_cmd"}, draw_cmd_buffer);
		AddInput<myvk_rg::Usage::kDrawIndirectBuffer>({"draw_count"}, draw_count_buffer);
	}
	inline void Update(const std::vector<ChunkMeshRendererBase::PreparedCluster> &prepared_clusters) {
		m_p_prepared_clusters = &prepared_clusters;
	}

	inline void CreatePipeline() final {
		auto pipeline_layout = myvk::PipelineLayout::Create(GetRenderGraphPtr()->GetDevicePtr(),
		                                                    {
		                                                        m_global_texture_ptr->GetDescriptorSetLayout(),
		                                                        m_camera_ptr->GetDescriptorSetLayout(),
		                                                        m_renderer_ptr->GetDescriptorSetLayout(),
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
		command_buffer->CmdBindDescriptorSets(
		    {m_global_texture_ptr->GetDescriptorSet(), m_camera_ptr->GetFrameDescriptorSet(0)}, m_pipeline);

		const auto &draw_cmd_buffer = GetInput({"draw_cmd"})->GetResource<myvk_rg::BufferBase>()->GetVkBuffer();
		const auto &draw_count_buffer = GetInput({"draw_count"})->GetResource<myvk_rg::BufferBase>()->GetVkBuffer();

		uint32_t id = 0, draw_cmd_size = m_renderer_ptr->GetMaxMeshes() * sizeof(VkDrawIndexedIndirectCommand);
		for (const auto &prepared_cluster : *m_p_prepared_clusters) {
			const auto &mesh_info_desc_set = prepared_cluster.m_cluster_ptr->GetMeshInfoDescriptorSet();
			command_buffer->CmdBindDescriptorSets({mesh_info_desc_set}, 2, m_pipeline);

			const auto &vertex_buffer = prepared_cluster.m_cluster_ptr->GetVertexBuffer();
			const auto &index_buffer = prepared_cluster.m_cluster_ptr->GetIndexBuffer();
			command_buffer->CmdBindVertexBuffer(vertex_buffer, 0);
			command_buffer->CmdBindIndexBuffer(index_buffer, 0, prepared_cluster.m_cluster_ptr->kIndexType);
			command_buffer->CmdDrawIndexedIndirectCount(draw_cmd_buffer, id * draw_cmd_size,      //
			                                            draw_count_buffer, id * sizeof(uint32_t), //
			                                            prepared_cluster.m_mesh_count);
			++id;
		}
	}
};

#endif
