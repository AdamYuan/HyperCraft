#include <client/ChunkWorker.hpp>

#include <client/World.hpp>
void ChunkWorker::push_worker(std::unique_ptr<ChunkWorker> &&worker) const {
	std::shared_ptr<World> world = m_chunk_ptr->LockWorld();
	if (world)
		world->PushWorker(std::move(worker));
}
