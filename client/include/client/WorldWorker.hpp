#pragma once

#include <client/World.hpp>

#include <atomic>
#include <memory>
#include <thread>
#include <vector>

namespace hc::client {

class WorldWorker : public std::enable_shared_from_this<WorldWorker> {
public:
	inline static std::shared_ptr<WorldWorker> Create(const std::shared_ptr<World> &world_ptr) {
		return std::make_shared<WorldWorker>(world_ptr);
	}

private:
	std::atomic_bool m_running{true};
	std::shared_ptr<World> m_world_ptr;
	std::vector<std::thread> m_worker_threads;

	void launch_worker_threads(std::size_t concurrency);
	void worker_thread_func();

public:
	inline explicit WorldWorker(std::shared_ptr<World> world_ptr) : m_world_ptr{std::move(world_ptr)} {}
	inline void Launch(std::size_t concurrency) {
		Join();
		launch_worker_threads(concurrency);
	}
	void Join();

	inline std::size_t GetConcurrency() const { return m_worker_threads.size(); }
};

} // namespace hc::client