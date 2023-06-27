#include <common/WorkPool.hpp>

namespace hc {

void WorkPool::Join() {
	m_worker_threads_running.store(false, std::memory_order_release);
	for (auto &i : m_worker_threads)
		i.join();
}

void WorkPool::launch_worker_threads(std::size_t concurrency) {
	m_worker_threads_running.store(true, std::memory_order_release);
	m_worker_threads.resize(concurrency);
	for (auto &i : m_worker_threads)
		i = std::thread(&WorkPool::worker_thread_func, this);
}

void WorkPool::worker_thread_func() {
	moodycamel::ConsumerToken consumer_token{m_workers};
	while (m_worker_threads_running.load(std::memory_order_acquire)) {
		std::unique_ptr<WorkerBase> worker{};

		if (m_workers.wait_dequeue_timed(consumer_token, worker, std::chrono::milliseconds(10))) {
			worker->Run();
			worker = nullptr;
		}
	}
}

} // namespace hc