#ifndef CUBECRAFT3_WORLDRENDERGRAPH_HPP
#define CUBECRAFT3_WORLDRENDERGRAPH_HPP

#include "ChunkCullPass.hpp"
#include "ChunkOpaquePass.hpp"
#include "ChunkTransparentPass.hpp"
#include "DepthHierarchyPass.hpp"
#include "FixTJunctionPass.hpp"
#include "OITBlendPass.hpp"

#include <myvk_rg/pass/ImGuiPass.hpp>
#include <myvk_rg/pass/ImageBlitPass.hpp>

#include <myvk_rg/resource/StaticBuffer.hpp>
#include <myvk_rg/resource/StaticImage.hpp>
#include <myvk_rg/resource/SwapchainImage.hpp>

class WorldRenderGraph final : public myvk_rg::RenderGraph<WorldRenderGraph> {
private:
	std::shared_ptr<ChunkMeshPool> m_chunk_mesh_pool_ptr;
	std::vector<std::shared_ptr<ChunkMeshCluster>> m_prepared_clusters;
	std::vector<std::unique_ptr<ChunkMeshPool::LocalUpdate>> m_post_updates;
	VkDeviceSize m_max_transfer_bytes{};

public:
	inline void SetTransferCapacity(VkDeviceSize max_transfer_bytes_per_sec, double delta) {
		m_max_transfer_bytes = VkDeviceSize((double)max_transfer_bytes_per_sec * delta);
	}
	inline void UpdateCamera(const std::shared_ptr<Camera> &camera_ptr) {
		camera_ptr->Update((Camera::UniformData *)GetResource<myvk_rg::StaticBuffer<myvk::Buffer>>({"camera"})
		                       ->GetBuffer()
		                       ->GetMappedData());
	}
	inline void UpdateDepthHierarchy() { GetPass<DepthHierarchyPass>({"depth_hierarchy_pass"})->UpdateLevelCount(); }
	inline void CmdUpdateChunkMesh(const myvk::Ptr<myvk::CommandBuffer> &command_buffer,
	                               std::vector<std::unique_ptr<ChunkMeshPool::LocalUpdate>> *p_post_updates,
	                               std::size_t post_update_threshold = 256u) {
		if (m_post_updates.size() >= post_update_threshold) {
			*p_post_updates = std::move(m_post_updates);
			m_post_updates.clear();
		} else
			p_post_updates->clear();

		m_chunk_mesh_pool_ptr->CmdLocalUpdate(command_buffer, &m_prepared_clusters, &m_post_updates,
		                                      m_max_transfer_bytes);
		GetResource<myvk_rg::StaticBuffer<ChunkMeshInfoBuffer>>({"chunk_mesh_info"})
		    ->GetBuffer()
		    ->Update(m_prepared_clusters);
		GetPass<ChunkCullPass>({"chunk_cull_pass"})->Update(m_prepared_clusters);
		GetPass<ChunkOpaquePass>({"chunk_opaque_pass"})->Update(m_prepared_clusters);
		GetPass<ChunkTransparentPass>({"chunk_transparent_pass"})->Update(m_prepared_clusters);
	}

	inline void Initialize(const myvk::Ptr<myvk::FrameManager> &frame_manager,
	                       const std::shared_ptr<ChunkMeshPool> &chunk_mesh_pool_ptr,
	                       const std::shared_ptr<GlobalTexture> &global_texture_ptr) {
		m_chunk_mesh_pool_ptr = chunk_mesh_pool_ptr;

		// Global Images
		auto block_texture_image = CreateResource<myvk_rg::StaticImage>(
		    {"block_texture"}, global_texture_ptr->GetBlockTextureView(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		auto light_map_image = CreateResource<myvk_rg::StaticImage>(
		    {"light_map"}, global_texture_ptr->GetLightMapView(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

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

		auto lf_depth_image = CreateResource<myvk_rg::LastFrameImage>({"lf_depth"});

		auto depth_hierarchy_pass = CreatePass<DepthHierarchyPass>({"depth_hierarchy_pass"}, lf_depth_image);

		auto chunk_cull_pass =
		    CreatePass<ChunkCullPass>({"chunk_cull_pass"}, chunk_mesh_pool_ptr, chunk_mesh_info_buffer, camera_buffer,
		                              depth_hierarchy_pass->GetDepthHierarchyOutput());
		auto chunk_opaque_pass = CreatePass<ChunkOpaquePass>({"chunk_opaque_pass"},                //
		                                                     block_texture_image, light_map_image, //
		                                                     chunk_mesh_info_buffer,               //
		                                                     camera_buffer,                        //
		                                                     color_image, depth_image,             //
		                                                     chunk_cull_pass->GetOpaqueDrawCmdOutput(),
		                                                     chunk_cull_pass->GetOpaqueDrawCountOutput());

		auto chunk_transparent_pass = CreatePass<ChunkTransparentPass>(
		    {"chunk_transparent_pass"},           //
		    block_texture_image, light_map_image, //
		    chunk_mesh_info_buffer,               //
		    camera_buffer,                        //
		    chunk_opaque_pass->GetDepthOutput(),  //
		    chunk_cull_pass->GetTransparentDrawCmdOutput(), chunk_cull_pass->GetTransparentDrawCountOutput());

		auto oit_blend_pass = CreatePass<OITBlendPass>({"oit_blend_pass"}, chunk_opaque_pass->GetColorOutput(),
		                                               chunk_transparent_pass->GetAccumOutput(),
		                                               chunk_transparent_pass->GetRevealOutput());

		auto swapchain_image = CreateResource<myvk_rg::SwapchainImage>({"swapchain_image"}, frame_manager);
		swapchain_image->SetLoadOp(VK_ATTACHMENT_LOAD_OP_DONT_CARE);

		auto fix_t_junction_pass =
		    CreatePass<FixTJunctionPass>({"fix_t_junction_pass"}, oit_blend_pass->GetColorOutput(),
		                                 chunk_opaque_pass->GetDepthOutput(), swapchain_image);

		lf_depth_image->SetCurrentResource(fix_t_junction_pass->GetFixedDepthOutput());

		// auto blit_pass = CreatePass<myvk_rg::ImageBlitPass>({"blit_pass"},
		// fix_t_junction_pass->GetFixedDepthOutput(),
		//                                                     swapchain_image, VK_FILTER_NEAREST);

		auto imgui_pass = CreatePass<myvk_rg::ImGuiPass>({"imgui_pass"}, fix_t_junction_pass->GetFixedColorOutput());

		AddResult({"final"}, imgui_pass->GetImageOutput());
	}
};

#endif
