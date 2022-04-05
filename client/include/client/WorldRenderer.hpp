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
	std::unique_ptr<ChunkRenderer> m_chunk_renderer;
	std::unique_ptr<OITCompositor> m_oit_compositor;

	// Child
	// std::shared_ptr<GlobalTexture> m_texture_ptr;
	// std::shared_ptr<Camera> m_camera_ptr;
	std::shared_ptr<myvk::FrameManager> m_frame_manager_ptr;

	std::shared_ptr<World> m_world_ptr;

	// Render pass
	std::shared_ptr<myvk::RenderPass> m_render_pass;

	// Frame
	std::shared_ptr<myvk::DescriptorPool> m_descriptor_pool;
	std::shared_ptr<myvk::DescriptorSetLayout> m_descriptor_set_layout;
	std::shared_ptr<myvk::Sampler> m_sampler; // result sampler for post-processing
	struct FrameData {
		std::shared_ptr<myvk::Image> result, depth, opaque, accum, reveal;
		std::shared_ptr<myvk::ImageView> result_view, depth_view, opaque_view, accum_view, reveal_view;
		std::shared_ptr<myvk::Framebuffer> framebuffer;

		std::shared_ptr<myvk::DescriptorSet> descriptor_set;
	};
	FrameData m_frames[kFrameCount];
	void create_render_pass();
	void create_frame_descriptors();
	void create_frame_images();

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
		// Create renderers
		ret->create_render_pass();
		ret->m_chunk_renderer = ChunkRenderer::Create(frame_manager_ptr, texture_ptr, camera_ptr, depth_ptr,
		                                              transfer_queue, ret->m_render_pass, 0, 1);
		ret->m_oit_compositor = OITCompositor::Create(ret->m_render_pass, 2);

		ret->m_sampler = myvk::Sampler::Create(transfer_queue->GetDevicePtr(), VK_FILTER_NEAREST,
		                                       VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);
		ret->create_frame_descriptors();
		ret->create_frame_images();
		return ret;
	}

	inline void Resize() { create_frame_images(); }

	inline const std::shared_ptr<World> &GetWorldPtr() const { return m_world_ptr; }
	inline const std::shared_ptr<myvk::Queue> &GetTransferQueue() const { return m_transfer_queue; }

	inline const std::unique_ptr<ChunkRenderer> &GetChunkRenderer() const { return m_chunk_renderer; }

	inline const std::shared_ptr<myvk::FrameManager> &GetFrameManagerPtr() const { return m_frame_manager_ptr; }

	void CmdRenderPass(const std::shared_ptr<myvk::CommandBuffer> &command_buffer) const;

	const std::shared_ptr<myvk::DescriptorSetLayout> &GetInputDescriptorLayout() const {
		return m_descriptor_set_layout;
	}
	const std::shared_ptr<myvk::DescriptorSet> &GetInputDescriptorSet() const {
		return m_frames[m_frame_manager_ptr->GetCurrentFrame()].descriptor_set;
	}
};

#endif
