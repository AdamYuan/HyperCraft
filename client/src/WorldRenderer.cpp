#include <client/WorldRenderer.hpp>

namespace hc::client {

void WorldRenderer::thread_func() {
	moodycamel::ConsumerToken consumer_token{m_post_update_queue};
	while (m_thread_running.load(std::memory_order_acquire)) {
		std::vector<ChunkMeshPool::LocalUpdateEntry> post_updates;

		if (m_post_update_queue.wait_dequeue_timed(consumer_token, post_updates, std::chrono::milliseconds(10))) {
			if (post_updates.empty()) {
				// purge unloaded chunk meshes
				auto center_pos = m_world_ptr->GetCenterChunkPos();
				auto unload_radius = m_world_ptr->GetUnloadChunkRadius();

				auto locked_meshes = m_chunk_mesh_map.lock_table();
				for (auto it = locked_meshes.begin(); it != locked_meshes.end();) {
					if (ChunkPosDistance2(center_pos, it->first) > uint32_t(unload_radius * unload_radius))
						it = locked_meshes.erase(it);
					else
						++it;
				}
			} else {
				m_chunk_mesh_pool->PostUpdate(std::move(post_updates));
				post_updates.clear();
			}
		}
	}
}

} // namespace hc::client
