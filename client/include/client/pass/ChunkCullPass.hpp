#ifndef CUBECRAFT3_CLIENT_PASS_CHUNKCULLPASS_HPP
#define CUBECRAFT3_CLIENT_PASS_CHUNKCULLPASS_HPP

#include <client/Camera.hpp>
#include <client/ChunkMesh.hpp>

#include <myvk_rg/RenderGraph.hpp>

class ChunkCullPass final : public myvk_rg::ComputePassBase {
private:
	std::shared_ptr<ChunkMeshRendererBase> m_renderer_ptr;

	const std::vector<ChunkMeshRendererBase::PreparedCluster> *m_p_prepared_clusters;

	myvk::Ptr<myvk::ComputePipeline> m_pipeline;
	uint32_t m_supported_clusters = 0u;

public:
	MYVK_RG_INLINE_INITIALIZER(const std::shared_ptr<ChunkMeshRendererBase> &renderer_ptr,
	                           myvk_rg::BufferInput camera_buffer, myvk_rg::ImageInput depth_hierarchy) {
		m_renderer_ptr = renderer_ptr;

		auto opaque_draw_cmd_buffer = CreateResource<myvk_rg::ManagedBuffer>({"opaque_draw_cmd"});
		auto opaque_draw_count_buffer = CreateResource<myvk_rg::ManagedBuffer>({"opaque_draw_count"});
		opaque_draw_count_buffer->SetMapType(myvk_rg::BufferMapType::kSeqWrite);

		auto trans_draw_cmd_buffer = CreateResource<myvk_rg::ManagedBuffer>({"trans_draw_cmd"});
		auto trans_draw_count_buffer = CreateResource<myvk_rg::ManagedBuffer>({"trans_draw_count"});
		trans_draw_count_buffer->SetMapType(myvk_rg::BufferMapType::kSeqWrite);

		AddDescriptorInput<0, myvk_rg::Usage::kStorageBufferW, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT>(
		    {"opaque_draw_cmd"}, opaque_draw_cmd_buffer);
		AddDescriptorInput<1, myvk_rg::Usage::kStorageBufferRW, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT>(
		    {"opaque_draw_count"}, opaque_draw_count_buffer);
		AddDescriptorInput<2, myvk_rg::Usage::kStorageBufferW, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT>(
		    {"trans_draw_cmd"}, trans_draw_cmd_buffer);
		AddDescriptorInput<3, myvk_rg::Usage::kStorageBufferRW, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT>(
		    {"trans_draw_count"}, trans_draw_count_buffer);
		AddDescriptorInput<4, myvk_rg::Usage::kUniformBuffer, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT>({"camera"},
		                                                                                              camera_buffer);
		// AddDescriptorInput<4, myvk_rg::Usage::kSampledImage, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT>(
		//     {"depth_hierarchy"}, depth_hierarchy, nullptr);
	}
	inline void Update(const std::vector<ChunkMeshRendererBase::PreparedCluster> &prepared_clusters) {
		m_p_prepared_clusters = &prepared_clusters;

		auto opaque_draw_cmd_buffer = GetResource<myvk_rg::ManagedBuffer>({"opaque_draw_cmd"});
		auto opaque_draw_count_buffer = GetResource<myvk_rg::ManagedBuffer>({"opaque_draw_count"});
		auto trans_draw_cmd_buffer = GetResource<myvk_rg::ManagedBuffer>({"trans_draw_cmd"});
		auto trans_draw_count_buffer = GetResource<myvk_rg::ManagedBuffer>({"trans_draw_count"});

		auto clusters = std::max((uint32_t)prepared_clusters.size(), 1u);
		if (m_supported_clusters < clusters) {
			m_supported_clusters = clusters;
			opaque_draw_cmd_buffer->SetSize(clusters * m_renderer_ptr->GetMaxMeshes() *
			                                sizeof(VkDrawIndexedIndirectCommand));
			opaque_draw_count_buffer->SetSize(clusters * sizeof(uint32_t));

			trans_draw_cmd_buffer->SetSize(clusters * m_renderer_ptr->GetMaxMeshes() *
			                               sizeof(VkDrawIndexedIndirectCommand));
			trans_draw_count_buffer->SetSize(clusters * sizeof(uint32_t));
		}
	}
	inline auto GetOpaqueDrawCmdOutput() { return MakeBufferOutput({"opaque_draw_cmd"}); }
	inline auto GetOpaqueDrawCountOutput() { return MakeBufferOutput({"opaque_draw_count"}); }
	inline auto GetTransparentDrawCmdOutput() { return MakeBufferOutput({"trans_draw_cmd"}); }
	inline auto GetTransparentDrawCountOutput() { return MakeBufferOutput({"trans_draw_count"}); }
	inline void CreatePipeline() final {
		auto pipeline_layout = myvk::PipelineLayout::Create(GetRenderGraphPtr()->GetDevicePtr(),
		                                                    {
		                                                        GetVkDescriptorSetLayout(),
		                                                        m_renderer_ptr->GetDescriptorSetLayout(),
		                                                    },
		                                                    {
		                                                        {VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(uint32_t) * 3},
		                                                    });
		static constexpr uint32_t kChunkCullingCompSpv[] = {
#include <client/shader/chunk_culling.comp.u32>
		};
		m_pipeline = myvk::ComputePipeline::Create(
		    pipeline_layout, myvk::ShaderModule::Create(GetRenderGraphPtr()->GetDevicePtr(), kChunkCullingCompSpv,
		                                                sizeof(kChunkCullingCompSpv)));
	}
	inline void CmdExecute(const myvk::Ptr<myvk::CommandBuffer> &command_buffer) const final {
		command_buffer->CmdBindPipeline(m_pipeline);
		command_buffer->CmdBindDescriptorSets({GetVkDescriptorSet()}, m_pipeline);
		// Clear counters
		uint32_t clusters = m_p_prepared_clusters->size();
		auto *opaque_draw_count_mapped =
		    GetResource<myvk_rg::ManagedBuffer>({"opaque_draw_count"})->GetMappedData<uint32_t>();
		auto *trans_draw_count_mapped =
		    GetResource<myvk_rg::ManagedBuffer>({"trans_draw_count"})->GetMappedData<uint32_t>();
		std::fill(opaque_draw_count_mapped, opaque_draw_count_mapped + clusters, 0u);
		std::fill(trans_draw_count_mapped, trans_draw_count_mapped + clusters, 0u);

		uint32_t count_offset = 0, draw_cmd_offset = 0, draw_cmd_size = m_renderer_ptr->GetMaxMeshes();
		for (const auto &prepared_cluster : *m_p_prepared_clusters) {
			const auto &mesh_info_desc_set = prepared_cluster.m_cluster_ptr->GetMeshInfoDescriptorSet();
			command_buffer->CmdBindDescriptorSets({mesh_info_desc_set}, 1, m_pipeline);
			uint32_t pc_data[3] = {prepared_cluster.m_mesh_count, draw_cmd_offset, count_offset};
			command_buffer->CmdPushConstants(m_pipeline->GetPipelineLayoutPtr(), VK_SHADER_STAGE_COMPUTE_BIT, 0,
			                                 sizeof(uint32_t) * 3, pc_data);
			command_buffer->CmdDispatch(group_x_64(prepared_cluster.m_mesh_count), 1, 1);
			++count_offset;
			draw_cmd_offset += draw_cmd_size;
		}
	}
};

#endif
