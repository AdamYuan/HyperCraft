#pragma once

#include <blockingconcurrentqueue.h>

#include <atomic>
#include <thread>
#include <vector>

#include "WorkerBase.hpp"

namespace hc {

class WorkPool : public std::enable_shared_from_this<WorkPool> {
public:
	inline static std::shared_ptr<WorkPool> Create(std::size_t concurrency) {
		return std::make_shared<WorkPool>(concurrency);
	}

private:
	std::atomic_bool m_worker_threads_running{true};
	moodycamel::BlockingConcurrentQueue<std::unique_ptr<WorkerBase>> m_workers;
	std::vector<std::thread> m_worker_threads;

	void launch_worker_threads(std::size_t concurrency);
	void worker_thread_func();

public:
	inline explicit WorkPool(std::size_t concurrency) { launch_worker_threads(concurrency); }
	inline void Relaunch(std::size_t concurrency) {
		Join();
		launch_worker_threads(concurrency);
	}
	void Join();

	inline void PushWorker(std::unique_ptr<WorkerBase> &&worker) { m_workers.enqueue(std::move(worker)); }
	inline void PushWorkers(std::vector<std::unique_ptr<WorkerBase>> &&workers) {
		m_workers.enqueue_bulk(std::make_move_iterator(workers.begin()), workers.size());
	}

	inline std::size_t GetConcurrency() const { return m_worker_threads.size(); }

	inline std::size_t GetApproxWorkerCount() const { return m_workers.size_approx(); }
};

} // namespace hc
