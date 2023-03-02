#ifndef CUBECRAFT3_CLIENT_PASS_CHUNKTRANSPARENTPASS_HPP
#define CUBECRAFT3_CLIENT_PASS_CHUNKTRANSPARENTPASS_HPP

#include <client/Camera.hpp>
#include <client/ChunkMesh.hpp>
#include <client/GlobalTexture.hpp>

#include <myvk_rg/RenderGraph.hpp>

class ChunkTransparentPass final : public myvk_rg::GraphicsPassBase {
private:
	std::shared_ptr<ChunkMeshRendererBase> m_renderer_ptr;
	std::shared_ptr<Camera> m_camera_ptr;
	std::shared_ptr<GlobalTexture> m_global_texture_ptr;

	const std::vector<ChunkMeshRendererBase::PreparedCluster> *m_p_prepared_clusters;

public:
	MYVK_RG_INLINE_INITIALIZER(const std::shared_ptr<ChunkMeshRendererBase> &renderer_ptr,
	                           const std::shared_ptr<Camera> &camera_ptr,
	                           const std::shared_ptr<GlobalTexture> &global_texture_ptr,
	                           myvk_rg::ImageInput depth_image, myvk_rg::BufferInput draw_cmd_buffer,
	                           myvk_rg::BufferInput draw_count_buffer) {
		m_renderer_ptr = renderer_ptr;
		m_camera_ptr = camera_ptr;
		m_global_texture_ptr = global_texture_ptr;

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
	inline void Update(const std::vector<ChunkMeshRendererBase::PreparedCluster> &prepared_clusters) {
		m_p_prepared_clusters = &prepared_clusters;
	}

	inline void CreatePipeline() final {
		auto pipeline_layout = myvk::PipelineLayout::Create(
			GetRenderGraphPtr()->GetDevicePtr(),
			{m_camera_ptr->GetDescriptorSetLayout(), m_global_texture_ptr->GetDescriptorSetLayout()}, {});
	}
	inline auto GetAccumOutput() { return MakeImageOutput({"accum"}); }
	inline auto GetRevealOutput() { return MakeImageOutput({"reveal"}); }
	inline void CmdExecute(const myvk::Ptr<myvk::CommandBuffer> &command_buffer) const final {}
};


#endif
