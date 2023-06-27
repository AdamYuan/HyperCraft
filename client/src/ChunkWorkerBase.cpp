#include <client/ChunkWorkerBase.hpp>

#include <client/World.hpp>

namespace hc::client {

void ChunkWorkerBase::try_push_worker(std::unique_ptr<ChunkWorkerBase> &&worker) const {
	if (!worker)
		return;
	std::shared_ptr<World> world = m_chunk_ptr->LockWorld();
	if (world)
		world->GetWorkPoolPtr()->PushWorker(std::move(worker));
}

} // namespace hc::client