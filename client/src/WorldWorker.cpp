#include <client/WorldWorker.hpp>

namespace hc::client {

void WorldWorker::Join() {
	m_running.store(false, std::memory_order_release);
	for (auto &i : m_worker_threads)
		i.join();
}

void WorldWorker::launch_worker_threads(std::size_t concurrency) {
	m_running.store(true, std::memory_order_release);
	m_worker_threads.resize(concurrency);
	for (auto &i : m_worker_threads)
		i = std::thread(&WorldWorker::worker_thread_func, this);
}

void WorldWorker::worker_thread_func() {
	ChunkTaskPoolProducerConfig producer_config{};
	producer_config.max_high_priority_tasks = 64;
	producer_config.max_tick_tasks = 64;
	producer_config.max_tasks = 256;

	ChunkTaskPoolToken token{&m_world_ptr->m_chunk_task_pool, producer_config};
	while (m_running.load(std::memory_order_acquire))
		m_world_ptr->m_chunk_task_pool.Run(&token);
}

} // namespace hc::client