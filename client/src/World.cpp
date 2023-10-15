#include <client/World.hpp>

#include <client/WorldRenderer.hpp>

namespace hc::client {

void World::update() {
	m_chunk_pool.Update();
	auto renderer = m_world_renderer_weak_ptr.lock();
	if (renderer)
		renderer->EraseUnloadedMeshes();
}

} // namespace hc::client
