#include <client/World.hpp>

void World::launch_chunk_threads() {
	m_chunk_threads.resize(std::thread::hardware_concurrency());
	for (auto &i : m_chunk_threads)
		i = std::thread(&World::chunk_thread_func, this);
}
void World::chunk_thread_func() {
	while (m_chunk_threads_run) {
		std::unique_ptr<ChunkWorker> worker;
		if (m_chunk_workers.try_dequeue(worker)) {
			worker->Run();
		}
	}
}

World::~World() {
	m_chunk_threads_run = false;
	for (auto &i : m_chunk_threads)
		i.join();
}
