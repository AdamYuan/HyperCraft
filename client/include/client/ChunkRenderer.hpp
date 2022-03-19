#ifndef CUBECRAFT3_CLIENT_CHUNK_RENDERER_HPP
#define CUBECRAFT3_CLIENT_CHUNK_RENDERER_HPP

#include <cinttypes>

#include <client/Camera.hpp>
#include <client/Canvas.hpp>
#include <client/ChunkMesh.hpp>
#include <client/DepthHierarchy.hpp>
#include <client/GlobalTexture.hpp>
#include <client/MeshRendererBase.hpp>
#include <common/AABB.hpp>
#include <common/Position.hpp>

#include <myvk/ComputePipeline.hpp>
#include <myvk/GraphicsPipeline.hpp>

class ChunkRenderer : public ChunkMeshRendererBase {
private:
	inline static constexpr uint32_t kClusterFaceCount = 4 * 1024 * 1024;

	// Child
	std::shared_ptr<GlobalTexture> m_texture_ptr;
	std::shared_ptr<Camera> m_camera_ptr;
	std::shared_ptr<DepthHierarchy> m_depth_ptr; // for Hi-Z Culling

	// Culling pipeline
	std::shared_ptr<myvk::PipelineLayout> m_culling_pipeline_layout;
	std::shared_ptr<myvk::ComputePipeline> m_culling_pipeline;
	void create_culling_pipeline(const std::shared_ptr<myvk::Device> &device);

	// Opaque & Transparent pipeline
	std::shared_ptr<myvk::PipelineLayout> m_pipeline_layout;
	std::shared_ptr<myvk::GraphicsPipeline> m_opaque_pipeline, m_transparent_pipeline;
	void create_pipeline_layout(const std::shared_ptr<myvk::Device> &device);
	void create_opaque_pipeline(const std::shared_ptr<myvk::RenderPass> &render_pass, uint32_t subpass);
	void create_transparent_pipeline(const std::shared_ptr<myvk::RenderPass> &render_pass, uint32_t subpass);

	// Frame
	std::vector<PreparedCluster> m_frame_prepared_clusters[kFrameCount];

public:
	explicit ChunkRenderer(const std::shared_ptr<GlobalTexture> &texture_ptr, const std::shared_ptr<Camera> &camera_ptr,
	                       const std::shared_ptr<DepthHierarchy> &depth_ptr,
	                       const std::shared_ptr<myvk::Queue> &transfer_queue, uint32_t opaque_subpass,
	                       uint32_t transparent_subpass)
	    : ChunkMeshRendererBase(transfer_queue, kClusterFaceCount * 4 * sizeof(ChunkMeshVertex),
	                            kClusterFaceCount * 6 * sizeof(uint16_t)),
	      m_texture_ptr{texture_ptr}, m_camera_ptr{camera_ptr}, m_depth_ptr{depth_ptr} {
		create_culling_pipeline(transfer_queue->GetDevicePtr());
		create_pipeline_layout(transfer_queue->GetDevicePtr());
		create_opaque_pipeline(depth_ptr->GetCanvasPtr()->GetRenderPass(), opaque_subpass);
		create_transparent_pipeline(depth_ptr->GetCanvasPtr()->GetRenderPass(), transparent_subpass);
	}

	inline static std::unique_ptr<ChunkRenderer> Create(const std::shared_ptr<GlobalTexture> &texture_ptr,
	                                                    const std::shared_ptr<Camera> &camera_ptr,
	                                                    const std::shared_ptr<DepthHierarchy> &depth_ptr,
	                                                    const std::shared_ptr<myvk::Queue> &transfer_queue,
	                                                    uint32_t opaque_subpass, uint32_t transparent_subpass) {
		return std::make_unique<ChunkRenderer>(texture_ptr, camera_ptr, depth_ptr, transfer_queue, opaque_subpass,
		                                       transparent_subpass);
	}

	void PrepareFrame();
	void CmdDispatch(const std::shared_ptr<myvk::CommandBuffer> &command_buffer);
	void CmdOpaqueDrawIndirect(const std::shared_ptr<myvk::CommandBuffer> &command_buffer);
	void CmdTransparentDrawIndirect(const std::shared_ptr<myvk::CommandBuffer> &command_buffer);
};

#endif
