#ifndef CUBECRAFT3_CLIENT_WORLD_RENDERER_HPP
#define CUBECRAFT3_CLIENT_WORLD_RENDERER_HPP

#include <client/Camera.hpp>
#include <client/ChunkMesh.hpp>
#include <client/Config.hpp>
#include <client/GlobalTexture.hpp>
#include <client/World.hpp>

#include <myvk/Buffer.hpp>
#include <myvk/CommandBuffer.hpp>
#include <myvk/DescriptorSet.hpp>
#include <myvk/GraphicsPipeline.hpp>

class WorldRenderer : public std::enable_shared_from_this<WorldRenderer> {
private:
	// Vulkan Queue
	std::shared_ptr<myvk::Queue> m_transfer_queue;

	// Chunk Meshes
	mutable std::unordered_map<ChunkPos3, std::shared_ptr<ChunkMesh>> m_chunk_meshes;
	mutable std::mutex m_chunk_meshes_mutex;
	void upload_chunk_mesh(const std::shared_ptr<Chunk> &chunk_ptr, const std::shared_ptr<ChunkMesh> &chunk_mesh_ptr);
	friend class ChunkMesh;

	// Pipeline
	std::shared_ptr<myvk::PipelineLayout> m_pipeline_layout;
	std::shared_ptr<myvk::GraphicsPipeline> m_pipeline;
	void create_pipeline(const std::shared_ptr<myvk::RenderPass> &render_pass, uint32_t subpass);

	// Child
	std::shared_ptr<GlobalTexture> m_texture_ptr;
	std::shared_ptr<Camera> m_camera_ptr;
	std::shared_ptr<World> m_world_ptr;

public:
	inline static std::shared_ptr<WorldRenderer>
	Create(const std::shared_ptr<World> &world_ptr, const std::shared_ptr<GlobalTexture> &texture_ptr,
	       const std::shared_ptr<Camera> &camera_ptr, const std::shared_ptr<myvk::Queue> &transfer_queue,
	       const std::shared_ptr<myvk::RenderPass> &render_pass, uint32_t subpass) {
		std::shared_ptr<WorldRenderer> ret = std::make_shared<WorldRenderer>();
		world_ptr->m_world_renderer_weak_ptr = ret->weak_from_this();
		ret->m_world_ptr = world_ptr;
		ret->m_texture_ptr = texture_ptr;
		ret->m_camera_ptr = camera_ptr;
		ret->m_transfer_queue = transfer_queue;
		ret->create_pipeline(render_pass, subpass);
		return ret;
	}

	inline const std::shared_ptr<World> &GetWorldPtr() const { return m_world_ptr; }
	inline const std::shared_ptr<myvk::Queue> &GetTransferQueue() const { return m_transfer_queue; }

	void CmdDrawPipeline(const std::shared_ptr<myvk::CommandBuffer> &command_buffer, const VkExtent2D &extent,
	                     uint32_t current_frame) const;
};

#endif
