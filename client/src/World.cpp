#include <client/World.hpp>

#include <spdlog/spdlog.h>

void World::launch_chunk_threads() {
	m_chunk_threads.resize(std::thread::hardware_concurrency());
	for (auto &i : m_chunk_threads)
		i = std::thread(&World::chunk_thread_func, this);
}
void World::chunk_thread_func() {
	moodycamel::ConsumerToken consumer_token{m_chunk_workers};
	std::unique_ptr<ChunkWorker> worker;
	while (m_chunk_threads_running.load(std::memory_order_acquire)) {
		if (m_chunk_workers.wait_dequeue_timed(consumer_token, worker, std::chrono::milliseconds(10)))
			worker->Run();
	}
}

void World::Join() {
	m_chunk_threads_running.store(false, std::memory_order_release);
	for (auto &i : m_chunk_threads)
		i.join();
}
