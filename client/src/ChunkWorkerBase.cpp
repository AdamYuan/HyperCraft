#include <client/ChunkWorkerBase.hpp>

#include <client/World.hpp>
void ChunkWorkerBase::push_worker(std::unique_ptr<ChunkWorkerBase> &&worker) const {
	std::shared_ptr<World> world = m_chunk_ptr->LockWorld();
	if (world)
		world->PushWorker(std::move(worker));
}
