#ifndef HYPERCRAFT_CLIENT_PASS_CHUNKCULLPASS_HPP
#define HYPERCRAFT_CLIENT_PASS_CHUNKCULLPASS_HPP

#include <client/ChunkMesh.hpp>

#include <myvk_rg/RenderGraph.hpp>

namespace hc::client::rg {

class ChunkCullPass final : public myvk_rg::ComputePassBase {
private:
	static inline constexpr uint32_t group_x_64(uint32_t x) { return (x >> 6u) + (((x & 0x3fu) > 0u) ? 1u : 0u); }

	std::shared_ptr<ChunkMeshPool> m_chunk_mesh_pool_ptr;

	const std::vector<std::shared_ptr<ChunkMeshCluster>> *m_p_prepared_clusters;

	myvk::Ptr<myvk::ComputePipeline> m_pipeline;

public:
	inline void Initialize(const std::shared_ptr<ChunkMeshPool> &chunk_mesh_pool_ptr,
	                       myvk_rg::BufferInput mesh_info_buffer, myvk_rg::BufferInput camera_buffer,
	                       myvk_rg::ImageInput depth_hierarchy) {
		m_chunk_mesh_pool_ptr = chunk_mesh_pool_ptr;

		auto opaque_draw_cmd_buffer = CreateResource<myvk_rg::ManagedBuffer>({"opaque_draw_cmd"});
		auto opaque_draw_count_buffer = CreateResource<myvk_rg::ManagedBuffer>({"opaque_draw_count"});
		opaque_draw_count_buffer->SetMapType(myvk_rg::BufferMapType::kSeqWrite);
		opaque_draw_cmd_buffer->SetSize(chunk_mesh_pool_ptr->GetMaxClusters() *
		                                chunk_mesh_pool_ptr->GetMaxMeshesPerCluster() *
		                                sizeof(VkDrawIndexedIndirectCommand));
		opaque_draw_count_buffer->SetSize(chunk_mesh_pool_ptr->GetMaxClusters() * sizeof(uint32_t));

		auto trans_draw_cmd_buffer = CreateResource<myvk_rg::ManagedBuffer>({"trans_draw_cmd"});
		auto trans_draw_count_buffer = CreateResource<myvk_rg::ManagedBuffer>({"trans_draw_count"});
		trans_draw_count_buffer->SetMapType(myvk_rg::BufferMapType::kSeqWrite);
		trans_draw_cmd_buffer->SetSize(chunk_mesh_pool_ptr->GetMaxClusters() *
		                               chunk_mesh_pool_ptr->GetMaxMeshesPerCluster() *
		                               sizeof(VkDrawIndexedIndirectCommand));
		trans_draw_count_buffer->SetSize(chunk_mesh_pool_ptr->GetMaxClusters() * sizeof(uint32_t));

		AddDescriptorInput<0, myvk_rg::Usage::kStorageBufferW, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT>(
			{"opaque_draw_cmd"}, opaque_draw_cmd_buffer);
		AddDescriptorInput<1, myvk_rg::Usage::kStorageBufferRW, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT>(
			{"opaque_draw_count"}, opaque_draw_count_buffer);
		AddDescriptorInput<2, myvk_rg::Usage::kStorageBufferW, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT>(
			{"trans_draw_cmd"}, trans_draw_cmd_buffer);
		AddDescriptorInput<3, myvk_rg::Usage::kStorageBufferRW, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT>(
			{"trans_draw_count"}, trans_draw_count_buffer);
		AddDescriptorInput<4, myvk_rg::Usage::kStorageBufferR, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT>(
			{"mesh_info"}, mesh_info_buffer);
		AddDescriptorInput<5, myvk_rg::Usage::kUniformBuffer, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT>({"camera"},
		                                                                                              camera_buffer);

		VkSamplerCreateInfo create_info = {VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
		create_info.magFilter = VK_FILTER_LINEAR;
		create_info.minFilter = VK_FILTER_LINEAR;
		create_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
		create_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		create_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		create_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		create_info.minLod = 0.0f;
		create_info.maxLod = VK_LOD_CLAMP_NONE;
		VkSamplerReductionModeCreateInfo reduction_create_info = {VK_STRUCTURE_TYPE_SAMPLER_REDUCTION_MODE_CREATE_INFO};
		reduction_create_info.reductionMode = VK_SAMPLER_REDUCTION_MODE_MAX;
		create_info.pNext = &reduction_create_info;
		AddDescriptorInput<6, myvk_rg::Usage::kSampledImage, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT>(
			{"depth_hierarchy"}, depth_hierarchy,
			myvk::Sampler::Create(GetRenderGraphPtr()->GetDevicePtr(), create_info));
	}
	inline void Update(const std::vector<std::shared_ptr<ChunkMeshCluster>> &prepared_clusters) {
		m_p_prepared_clusters = &prepared_clusters;
	}
	inline auto GetOpaqueDrawCmdOutput() { return MakeBufferOutput({"opaque_draw_cmd"}); }
	inline auto GetOpaqueDrawCountOutput() { return MakeBufferOutput({"opaque_draw_count"}); }
	inline auto GetTransparentDrawCmdOutput() { return MakeBufferOutput({"trans_draw_cmd"}); }
	inline auto GetTransparentDrawCountOutput() { return MakeBufferOutput({"trans_draw_count"}); }
	inline void CreatePipeline() final {
		auto pipeline_layout =
			myvk::PipelineLayout::Create(GetRenderGraphPtr()->GetDevicePtr(), {GetVkDescriptorSetLayout()},
			                             {{VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(uint32_t) * 3}});
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
		auto *opaque_draw_count_mapped =
			GetResource<myvk_rg::ManagedBuffer>({"opaque_draw_count"})->GetMappedData<uint32_t>();
		auto *trans_draw_count_mapped =
			GetResource<myvk_rg::ManagedBuffer>({"trans_draw_count"})->GetMappedData<uint32_t>();

		std::fill(opaque_draw_count_mapped, opaque_draw_count_mapped + m_chunk_mesh_pool_ptr->GetMaxClusters(), 0u);
		std::fill(trans_draw_count_mapped, trans_draw_count_mapped + m_chunk_mesh_pool_ptr->GetMaxClusters(), 0u);

		for (const auto &cluster : *m_p_prepared_clusters) {
			uint32_t pc_data[] = {cluster->GetLocalMeshCount(),
			                      cluster->GetClusterOffset() * m_chunk_mesh_pool_ptr->GetMaxMeshesPerCluster(),
			                      cluster->GetClusterOffset()};
			command_buffer->CmdPushConstants(m_pipeline->GetPipelineLayoutPtr(), VK_SHADER_STAGE_COMPUTE_BIT, 0,
			                                 sizeof(uint32_t) * 3, pc_data);
			command_buffer->CmdDispatch(group_x_64(cluster->GetLocalMeshCount()), 1, 1);
		}
	}
};

}

#endif
