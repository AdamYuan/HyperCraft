#ifndef CUBECRAFT3_CLIENT_WORLD_RENDERER_HPP
#define CUBECRAFT3_CLIENT_WORLD_RENDERER_HPP

#include <client/Camera.hpp>
#include <client/ChunkMesh.hpp>
#include <client/ChunkRenderer.hpp>
#include <client/Config.hpp>
#include <client/DepthHierarchy.hpp>
#include <client/GlobalTexture.hpp>
#include <client/OITCompositor.hpp>
#include <client/World.hpp>

#include <myvk/Buffer.hpp>
#include <myvk/CommandBuffer.hpp>
#include <myvk/DescriptorSet.hpp>
#include <myvk/GraphicsPipeline.hpp>

#include <queue>
#include <unordered_map>
#include <vector>

class WorldRenderer : public std::enable_shared_from_this<WorldRenderer> {
private:
	// Vulkan Queue
	std::shared_ptr<myvk::Queue> m_transfer_queue;

	// Renderers
	std::shared_ptr<ChunkRenderer> m_chunk_renderer;

	// Child
	std::shared_ptr<myvk::FrameManager> m_frame_manager_ptr;

	std::shared_ptr<World> m_world_ptr;

	// Render pass
	std::shared_ptr<myvk::RenderPass> m_render_pass;

public:
	inline static std::shared_ptr<WorldRenderer>
	Create(const std::shared_ptr<myvk::FrameManager> &frame_manager_ptr, const std::shared_ptr<World> &world_ptr,
	       const std::shared_ptr<GlobalTexture> &texture_ptr, const std::shared_ptr<Camera> &camera_ptr,
	       const std::shared_ptr<DepthHierarchy> &depth_ptr, const std::shared_ptr<myvk::Queue> &transfer_queue) {
		std::shared_ptr<WorldRenderer> ret = std::make_shared<WorldRenderer>();
		world_ptr->m_world_renderer_weak_ptr = ret->weak_from_this();
		ret->m_world_ptr = world_ptr;
		ret->m_transfer_queue = transfer_queue;
		ret->m_frame_manager_ptr = frame_manager_ptr;
		ret->m_chunk_renderer =
		    ChunkRenderer::Create(frame_manager_ptr, texture_ptr, camera_ptr, depth_ptr, ret->m_render_pass, 0, 1);
		return ret;
	}

	inline const std::shared_ptr<World> &GetWorldPtr() const { return m_world_ptr; }
	inline const std::shared_ptr<myvk::Queue> &GetTransferQueue() const { return m_transfer_queue; }

	inline const auto &GetChunkRenderer() const { return m_chunk_renderer; }

	inline const std::shared_ptr<myvk::FrameManager> &GetFrameManagerPtr() const { return m_frame_manager_ptr; }

	void CmdRenderPass(const std::shared_ptr<myvk::CommandBuffer> &command_buffer) const;
};

#endif
