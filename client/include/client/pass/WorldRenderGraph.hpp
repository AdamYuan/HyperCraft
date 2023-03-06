#ifndef CUBECRAFT3_WORLDRENDERGRAPH_HPP
#define CUBECRAFT3_WORLDRENDERGRAPH_HPP

#include "ChunkCullPass.hpp"
#include "ChunkOpaquePass.hpp"

#include <myvk_rg/pass/ImGuiPass.hpp>
#include <myvk_rg/pass/ImageBlitPass.hpp>

#include <myvk_rg/resource/StaticBuffer.hpp>
#include <myvk_rg/resource/SwapchainImage.hpp>

class WorldRenderGraph final : public myvk_rg::RenderGraph<WorldRenderGraph> {
private:
	std::shared_ptr<ChunkMeshPool> m_chunk_mesh_pool_ptr;
	std::vector<std::shared_ptr<ChunkMeshCluster>> m_prepared_clusters;
	std::vector<std::unique_ptr<ChunkMeshPool::LocalUpdate>> m_post_updates;

public:
	inline void UpdateCamera(const std::shared_ptr<Camera> &camera_ptr) {
		camera_ptr->Update((Camera::UniformData *)GetResource<myvk_rg::StaticBuffer<myvk::Buffer>>({"camera"})
		                       ->GetBuffer()
		                       ->GetMappedData());
	}
	inline void CmdUpdateChunkMesh(const myvk::Ptr<myvk::CommandBuffer> &command_buffer) {
		m_chunk_mesh_pool_ptr->CmdLocalUpdate(command_buffer, &m_prepared_clusters, &m_post_updates, 4 * 1024, 64);
		GetResource<myvk_rg::StaticBuffer<ChunkMeshInfoBuffer>>({"chunk_mesh_info"})
		    ->GetBuffer()
		    ->Update(m_prepared_clusters);
		GetPass<ChunkCullPass>({"chunk_cull_pass"})->Update(m_prepared_clusters);
		GetPass<ChunkOpaquePass>({"chunk_opaque_pass"})->Update(m_prepared_clusters);
	}

	MYVK_RG_INLINE_INITIALIZER(const myvk::Ptr<myvk::FrameManager> &frame_manager,
	                           const std::shared_ptr<ChunkMeshPool> &chunk_mesh_pool_ptr,
	                           const std::shared_ptr<GlobalTexture> &global_texture_ptr) {
		m_chunk_mesh_pool_ptr = chunk_mesh_pool_ptr;

		// Create Camera Buffer
		auto camera_buffer = CreateResource<myvk_rg::StaticBuffer<myvk::Buffer>>(
		    {"camera"}, myvk::Buffer::Create(GetDevicePtr(), sizeof(Camera::UniformData),
		                                     VMA_ALLOCATION_CREATE_MAPPED_BIT |
		                                         VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
		                                     VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_AUTO));

		auto chunk_mesh_info_buffer = CreateResource<myvk_rg::StaticBuffer<ChunkMeshInfoBuffer>>(
		    {"chunk_mesh_info"}, ChunkMeshInfoBuffer::Create(chunk_mesh_pool_ptr, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT));

		auto format = VK_FORMAT_B10G11R11_UFLOAT_PACK32;

		auto color_image = CreateResource<myvk_rg::ManagedImage>({"color"}, format);
		color_image->SetLoadOp(VK_ATTACHMENT_LOAD_OP_CLEAR);
		color_image->SetClearColorValue({1.0, 1.0, 1.0, 1});

		auto depth_image = CreateResource<myvk_rg::ManagedImage>({"depth"}, VK_FORMAT_D32_SFLOAT);
		depth_image->SetLoadOp(VK_ATTACHMENT_LOAD_OP_CLEAR);
		depth_image->SetClearColorValue({1.0f});

		auto chunk_cull_pass = CreatePass<ChunkCullPass>({"chunk_cull_pass"}, chunk_mesh_pool_ptr,
		                                                 chunk_mesh_info_buffer, camera_buffer, nullptr);
		auto chunk_opaque_pass = CreatePass<ChunkOpaquePass>(
		    {"chunk_opaque_pass"}, chunk_mesh_pool_ptr, global_texture_ptr, chunk_mesh_info_buffer, camera_buffer,
		    color_image, depth_image, chunk_cull_pass->GetOpaqueDrawCmdOutput(),
		    chunk_cull_pass->GetOpaqueDrawCountOutput());

		auto swapchain_image = CreateResource<myvk_rg::SwapchainImage>({"swapchain_image"}, frame_manager);
		swapchain_image->SetLoadOp(VK_ATTACHMENT_LOAD_OP_DONT_CARE);

		auto copy_pass = CreatePass<myvk_rg::ImageBlitPass>({"blit_pass"}, chunk_opaque_pass->GetOpaqueOutput(),
		                                                    swapchain_image, VK_FILTER_NEAREST);

		auto imgui_pass = CreatePass<myvk_rg::ImGuiPass>({"imgui_pass"}, copy_pass->GetDstOutput());

		AddResult({"final"}, imgui_pass->GetImageOutput());
	}
};

#endif
