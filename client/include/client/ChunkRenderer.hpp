#ifndef CUBECRAFT3_CLIENT_CHUNK_RENDERER_HPP
#define CUBECRAFT3_CLIENT_CHUNK_RENDERER_HPP

#include <cinttypes>

#include <client/Camera.hpp>
#include <client/ChunkMesh.hpp>
#include <client/DepthHierarchy.hpp>
#include <client/GlobalTexture.hpp>
#include <client/MeshRendererBase.hpp>
#include <common/AABB.hpp>
#include <common/Position.hpp>

#include <myvk/ComputePipeline.hpp>
#include <myvk/GraphicsPipeline.hpp>

#include <myvk_rg/RenderGraph.hpp>

class ChunkRenderer : public ChunkMeshRendererBase {
private:
	inline static constexpr uint32_t kClusterFaceCount = 4 * 1024 * 1024;

	// Child
	std::shared_ptr<GlobalTexture> m_texture_ptr;
	std::shared_ptr<Camera> m_camera_ptr;
	std::shared_ptr<DepthHierarchy> m_depth_ptr; // for Hi-Z Culling

	std::shared_ptr<myvk::RenderPass> m_render_pass_ptr;
	std::shared_ptr<myvk::FrameManager> m_frame_manager_ptr;

public:
	explicit ChunkRenderer(const std::shared_ptr<myvk::FrameManager> &frame_manager_ptr,
	                       const std::shared_ptr<GlobalTexture> &texture_ptr, const std::shared_ptr<Camera> &camera_ptr,
	                       const std::shared_ptr<DepthHierarchy> &depth_ptr,
	                       const std::shared_ptr<myvk::RenderPass> &render_pass_ptr, uint32_t opaque_subpass,
	                       uint32_t transparent_subpass)
	    : ChunkMeshRendererBase(frame_manager_ptr->GetDevicePtr(), kClusterFaceCount * 4 * sizeof(ChunkMeshVertex),
	                            kClusterFaceCount * 6 * sizeof(uint16_t), 15u),
	      m_render_pass_ptr{render_pass_ptr}, m_frame_manager_ptr{frame_manager_ptr}, m_texture_ptr{texture_ptr},
	      m_camera_ptr{camera_ptr}, m_depth_ptr{depth_ptr} {
	}

	inline static std::shared_ptr<ChunkRenderer> Create(const std::shared_ptr<myvk::FrameManager> &frame_manager_ptr,
	                                                    const std::shared_ptr<GlobalTexture> &texture_ptr,
	                                                    const std::shared_ptr<Camera> &camera_ptr,
	                                                    const std::shared_ptr<DepthHierarchy> &depth_ptr,
	                                                    const std::shared_ptr<myvk::RenderPass> &render_pass_ptr,
	                                                    uint32_t opaque_subpass, uint32_t transparent_subpass) {
		return std::make_shared<ChunkRenderer>(frame_manager_ptr, texture_ptr, camera_ptr, depth_ptr, render_pass_ptr,
		                                       opaque_subpass, transparent_subpass);
	}

	inline const std::shared_ptr<DepthHierarchy> &GetDepthHierarchyPtr() const { return m_depth_ptr; }
};

#endif
