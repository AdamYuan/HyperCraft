#ifndef WORLD_RENDERER_HPP
#define WORLD_RENDERER_HPP

#include <client/Camera.hpp>
#include <client/Config.hpp>
#include <client/GlobalTexture.hpp>
#include <client/World.hpp>

#include <shared_mutex>

#include <myvk/Buffer.hpp>
#include <myvk/CommandBuffer.hpp>
#include <myvk/DescriptorSet.hpp>
#include <myvk/GraphicsPipeline.hpp>

class WorldRenderer : public std::enable_shared_from_this<WorldRenderer> {
private:
	// Vulkan Queue
	std::shared_ptr<myvk::Queue> m_transfer_queue;

	// Draw Commands
	struct DrawCmd {
		std::shared_ptr<myvk::Buffer> m_vertex_buffer, m_index_buffer;
		std::shared_ptr<myvk::Buffer> m_frame_vertices[kFrameCount], m_frame_indices[kFrameCount];
	};
	mutable std::unordered_map<glm::i16vec3, DrawCmd> m_draw_commands;
	mutable std::shared_mutex m_draw_cmd_mutex;
	void push_draw_cmd(const std::vector<Chunk::Vertex> &vertices, const std::vector<uint16_t> &indices,
	                   DrawCmd *draw_cmd) const;

	// Pipeline
	std::shared_ptr<myvk::PipelineLayout> m_pipeline_layout;
	std::shared_ptr<myvk::GraphicsPipeline> m_pipeline;
	void create_pipeline(const std::shared_ptr<myvk::RenderPass> &render_pass, uint32_t subpass);

	// Child
	std::shared_ptr<GlobalTexture> m_texture;
	std::shared_ptr<Camera> m_camera;
	std::shared_ptr<World> m_world;

public:
	inline static std::shared_ptr<WorldRenderer>
	Create(const std::shared_ptr<World> &world, const std::shared_ptr<GlobalTexture> &texture,
	       const std::shared_ptr<Camera> &camera, const std::shared_ptr<myvk::Queue> &transfer_queue,
	       const std::shared_ptr<myvk::RenderPass> &render_pass, uint32_t subpass) {
		std::shared_ptr<WorldRenderer> ret = std::make_shared<WorldRenderer>();
		world->m_world_renderer = ret->weak_from_this();
		ret->m_world = world;
		ret->m_texture = texture;
		ret->m_camera = camera;
		ret->m_transfer_queue = transfer_queue;
		ret->create_pipeline(render_pass, subpass);
		return ret;
	}

	void UploadMesh(const std::shared_ptr<Chunk> &chunk); // A thread-safe function for mesh uploading

	inline const std::shared_ptr<World> &GetWorldPtr() const { return m_world; }

	void CmdDrawPipeline(const std::shared_ptr<myvk::CommandBuffer> &command_buffer, const VkExtent2D &extent,
	                     uint32_t current_frame) const;
};

#endif
