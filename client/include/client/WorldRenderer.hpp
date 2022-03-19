#ifndef CUBECRAFT3_CLIENT_WORLD_RENDERER_HPP
#define CUBECRAFT3_CLIENT_WORLD_RENDERER_HPP

#include <client/Camera.hpp>
#include <client/Canvas.hpp>
#include <client/ChunkMesh.hpp>
#include <client/ChunkRenderer.hpp>
#include <client/Config.hpp>
#include <client/DepthHierarchy.hpp>
#include <client/GlobalTexture.hpp>
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
	std::unique_ptr<ChunkRenderer> m_chunk_renderer;

	// Child
	// std::shared_ptr<GlobalTexture> m_texture_ptr;
	// std::shared_ptr<Camera> m_camera_ptr;
	std::shared_ptr<Canvas> m_canvas_ptr;

	std::shared_ptr<World> m_world_ptr;

	// Screen pass
	std::shared_ptr<myvk::DescriptorPool> m_screen_descriptor_pool;
	std::shared_ptr<myvk::DescriptorSetLayout> m_screen_descriptor_set_layout;
	std::vector<std::shared_ptr<myvk::DescriptorSet>> m_screen_descriptor_sets;
	std::shared_ptr<myvk::PipelineLayout> m_screen_pipeline_layout;
	std::shared_ptr<myvk::GraphicsPipeline> m_screen_pipeline;

	void create_descriptors();
	void update_descriptors();
	void create_pipeline(uint32_t subpass);

public:
	inline static std::shared_ptr<WorldRenderer>
	Create(const std::shared_ptr<World> &world_ptr, const std::shared_ptr<GlobalTexture> &texture_ptr,
	       const std::shared_ptr<Camera> &camera_ptr, const std::shared_ptr<DepthHierarchy> &depth_ptr,
	       const std::shared_ptr<myvk::Queue> &transfer_queue, uint32_t chunk_opaque_subpass,
	       uint32_t chunk_transparent_subpass, uint32_t screen_subpass) {
		std::shared_ptr<WorldRenderer> ret = std::make_shared<WorldRenderer>();
		world_ptr->m_world_renderer_weak_ptr = ret->weak_from_this();
		ret->m_world_ptr = world_ptr;
		ret->m_transfer_queue = transfer_queue;
		ret->m_canvas_ptr = depth_ptr->GetCanvasPtr();
		// Create renderers
		ret->m_chunk_renderer = ChunkRenderer::Create(texture_ptr, camera_ptr, depth_ptr, transfer_queue,
		                                              chunk_opaque_subpass, chunk_transparent_subpass);
		ret->create_descriptors();
		ret->update_descriptors();
		ret->create_pipeline(screen_subpass);
		return ret;
	}

	inline void Resize() { update_descriptors(); }

	inline const std::shared_ptr<World> &GetWorldPtr() const { return m_world_ptr; }
	inline const std::shared_ptr<myvk::Queue> &GetTransferQueue() const { return m_transfer_queue; }

	inline const std::unique_ptr<ChunkRenderer> &GetChunkRenderer() const { return m_chunk_renderer; }

	void CmdScreenDraw(const std::shared_ptr<myvk::CommandBuffer> &command_buffer) const;

	/* inline void CmdRender(const std::shared_ptr<myvk::CommandBuffer> &command_buffer, const VkExtent2D &extent,
	                      uint32_t current_frame) const {
	    m_chunk_renderer->CmdRender(command_buffer, extent, current_frame);
	}*/
};

#endif
