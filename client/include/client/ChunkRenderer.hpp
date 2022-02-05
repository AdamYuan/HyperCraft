#ifndef CUBECRAFT3_CLIENT_CHUNK_RENDERER_HPP
#define CUBECRAFT3_CLIENT_CHUNK_RENDERER_HPP

#include <cinttypes>

#include <client/Camera.hpp>
#include <client/Canvas.hpp>
#include <client/ChunkMesh.hpp>
#include <client/GlobalTexture.hpp>
#include <client/MeshRendererBase.hpp>
#include <common/AABB.hpp>
#include <common/Position.hpp>

#include <myvk/ComputePipeline.hpp>
#include <myvk/GraphicsPipeline.hpp>

class ChunkRenderer : public MeshRendererBase<ChunkMeshVertex, uint16_t, ChunkMeshInfo> {
private:
	inline static constexpr uint32_t kClusterFaceCount = 4 * 1024 * 1024;

	// Child
	std::shared_ptr<GlobalTexture> m_texture_ptr;
	std::shared_ptr<Camera> m_camera_ptr;
	std::shared_ptr<Canvas> m_canvas_ptr;

	// Culling pipeline
	std::shared_ptr<myvk::PipelineLayout> m_culling_pipeline_layout;
	std::shared_ptr<myvk::ComputePipeline> m_culling_pipeline;
	void create_culling_pipeline(const std::shared_ptr<myvk::Device> &device);

	// Main pipeline
	std::shared_ptr<myvk::PipelineLayout> m_main_pipeline_layout;
	std::shared_ptr<myvk::GraphicsPipeline> m_main_pipeline;
	void create_main_pipeline(const std::shared_ptr<myvk::RenderPass> &render_pass, uint32_t subpass);

	// Frame
	std::vector<PreparedCluster> m_prepared_cluster_vector;

public:
	explicit ChunkRenderer(const std::shared_ptr<GlobalTexture> &texture_ptr, const std::shared_ptr<Camera> &camera_ptr,
	                       const std::shared_ptr<Canvas> &canvas_ptr,
	                       const std::shared_ptr<myvk::Queue> &transfer_queue, uint32_t subpass)
	    : MeshRendererBase<ChunkMeshVertex, uint16_t, ChunkMeshInfo>(transfer_queue,
	                                                                 kClusterFaceCount * 4 * sizeof(ChunkMeshVertex),
	                                                                 kClusterFaceCount * 6 * sizeof(uint16_t)),
	      m_texture_ptr{texture_ptr}, m_camera_ptr{camera_ptr}, m_canvas_ptr{canvas_ptr} {
		create_culling_pipeline(transfer_queue->GetDevicePtr());
		create_main_pipeline(canvas_ptr->GetRenderPass(), subpass);
	}

	inline static std::unique_ptr<ChunkRenderer> Create(const std::shared_ptr<GlobalTexture> &texture_ptr,
	                                                    const std::shared_ptr<Camera> &camera_ptr,
	                                                    const std::shared_ptr<Canvas> &canvas_ptr,
	                                                    const std::shared_ptr<myvk::Queue> &transfer_queue,
	                                                    uint32_t subpass) {
		return std::make_unique<ChunkRenderer>(texture_ptr, camera_ptr, canvas_ptr, transfer_queue, subpass);
	}

	void BeginFrame(uint32_t current_frame);
	void CmdDispatch(const std::shared_ptr<myvk::CommandBuffer> &command_buffer, uint32_t current_frame);
	void CmdDrawIndirect(const std::shared_ptr<myvk::CommandBuffer> &command_buffer, const VkExtent2D &extent,
	                     uint32_t current_frame);
	inline void EndFrame() { m_prepared_cluster_vector.clear(); }
};

#endif
