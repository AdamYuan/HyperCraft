#ifndef HYPERCRAFT_CLIENT_WORLD_RENDERER_HPP
#define HYPERCRAFT_CLIENT_WORLD_RENDERER_HPP

#include <client/Camera.hpp>
#include <client/ChunkMesh.hpp>
#include <client/Config.hpp>
#include <client/GlobalTexture.hpp>
#include <client/World.hpp>

#include <myvk/Buffer.hpp>
#include <myvk/CommandBuffer.hpp>
#include <myvk/DescriptorSet.hpp>
#include <myvk/GraphicsPipeline.hpp>

#include <concurrentqueue.h>
#include <cuckoohash_map.hh>
#include <queue>
#include <unordered_map>
#include <vector>

namespace hc::client {

class WorldRenderer : public std::enable_shared_from_this<WorldRenderer> {
private:
	std::shared_ptr<ChunkMeshPool> m_chunk_mesh_pool;
	std::shared_ptr<World> m_world_ptr;
	libcuckoo::cuckoohash_map<ChunkPos3, std::vector<std::unique_ptr<ChunkMeshHandle>>> m_chunk_mesh_map;

	// Mesh local update
	VkDeviceSize m_max_transfer_bytes{};

	// Background thread
	std::thread m_thread;
	std::atomic_bool m_thread_running;
	moodycamel::ConcurrentQueue<std::vector<ChunkMeshPool::PostUpdateEntry>> m_post_update_queue;

	void thread_func();

public:
	inline static std::shared_ptr<WorldRenderer> Create(const myvk::Ptr<myvk::Device> &device,
	                                                    const std::shared_ptr<World> &world_ptr) {
		auto ret = std::make_shared<WorldRenderer>(device, world_ptr);
		world_ptr->m_world_renderer_weak_ptr = ret;
		return ret;
	}

	inline WorldRenderer(const myvk::Ptr<myvk::Device> &device, const std::shared_ptr<World> &world_ptr)
	    : m_thread_running{true} {
		m_world_ptr = world_ptr;
		m_chunk_mesh_pool = ChunkMeshPool::Create(device);
		m_thread = std::thread{&WorldRenderer::thread_func, this};
	}

	inline ~WorldRenderer() {
		auto locked_meshes = m_chunk_mesh_map.lock_table();
		for (auto &i : locked_meshes) {
			for (auto &m : i.second)
				m->SetFinalize();
		}
	}

	inline void Join() {
		m_thread_running.store(false, std::memory_order_release);
		m_thread.join();
	}

	inline const std::shared_ptr<World> &GetWorldPtr() const { return m_world_ptr; }

	inline void PushChunkMesh(const ChunkPos3 &chunk_pos, std::vector<BlockMesh> &&meshes) {
		ChunkMeshHandleTransaction transaction{m_chunk_mesh_pool};

		glm::i32vec3 base_position = (glm::i32vec3)chunk_pos * (int32_t)Chunk::kSize;
		std::vector<std::unique_ptr<ChunkMeshHandle>> mesh_handles(meshes.size());
		for (uint32_t i = 0; i < meshes.size(); ++i) {
			auto &info = meshes[i];
			mesh_handles[i] = transaction.Create(
			    info.vertices, info.indices,
			    {(fAABB)info.aabb / glm::vec3(1u << BlockVertex::kUnitBitOffset) + (glm::vec3)base_position,
			     base_position, (uint32_t)info.transparent});
		}
		m_chunk_mesh_map.uprase_fn(chunk_pos, [&mesh_handles](auto &data, libcuckoo::UpsertContext) {
			std::swap(data, mesh_handles);
			return false;
		});
		for (auto &handle : mesh_handles)
			transaction.Destroy(std::move(handle));
	}

	inline void SetTransferCapacity(VkDeviceSize max_transfer_bytes_per_sec, double delta) {
		m_max_transfer_bytes = VkDeviceSize((double)max_transfer_bytes_per_sec * delta);
	}

	inline void CmdUpdateClusters(const myvk::Ptr<myvk::CommandBuffer> &command_buffer,
	                              std::vector<std::shared_ptr<ChunkMeshCluster>> *p_prepared_clusters,
	                              std::vector<ChunkMeshPool::PostUpdateEntry> *p_post_updates) {
		m_chunk_mesh_pool->CmdLocalUpdate(command_buffer, p_prepared_clusters, p_post_updates, m_max_transfer_bytes);
	}

	inline void PushPostUpdates(std::vector<ChunkMeshPool::PostUpdateEntry> &&post_updates) {
		if (post_updates.empty())
			return;
		m_post_update_queue.enqueue(std::move(post_updates));
	}

	inline void EraseUnloadedMeshes() { m_post_update_queue.enqueue({}); }

	inline myvk::Ptr<ChunkMeshInfoBuffer> CreateChunkMeshInfoBuffer(VkBufferUsageFlags usages) {
		return ChunkMeshInfoBuffer::Create(m_chunk_mesh_pool, usages);
	}
};

} // namespace hc::client

#endif
