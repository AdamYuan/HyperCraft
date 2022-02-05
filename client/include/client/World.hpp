#ifndef CUBECRAFT3_CLIENT_WORLD_HPP
#define CUBECRAFT3_CLIENT_WORLD_HPP

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <blockingconcurrentqueue.h>

#include <atomic>
#include <thread>
#include <unordered_map>
#include <vector>

#include <client/Chunk.hpp>
#include <client/WorkerBase.hpp>

class WorldRenderer;
class ClientBase;

class World : public std::enable_shared_from_this<World> {
public:
	inline static std::shared_ptr<World> Create() { return std::make_shared<World>(); }

private:
	// Parent weak_ptrs
	std::weak_ptr<WorldRenderer> m_world_renderer_weak_ptr;
	friend class WorldRenderer;
	std::weak_ptr<ClientBase> m_client_weak_ptr;
	friend class LocalClient;
	friend class ENetClient;

	// Chunks
	std::unordered_map<ChunkPos3, std::shared_ptr<Chunk>> m_chunks;

	// Chunk workers
	std::atomic_bool m_worker_threads_running{true};
	moodycamel::BlockingConcurrentQueue<std::unique_ptr<WorkerBase>> m_workers;
	std::vector<std::thread> m_worker_threads;

	void launch_worker_threads();
	void worker_thread_func();

public:
	inline const std::weak_ptr<WorldRenderer> &GetWorldRendererWeakPtr() const { return m_world_renderer_weak_ptr; }
	inline std::shared_ptr<WorldRenderer> LockWorldRenderer() const { return m_world_renderer_weak_ptr.lock(); }

	inline const std::weak_ptr<ClientBase> &GetClientWeakPtr() const { return m_client_weak_ptr; }
	inline std::shared_ptr<ClientBase> LockClient() const { return m_client_weak_ptr.lock(); }

	inline void PushWorker(std::unique_ptr<WorkerBase> &&worker) { m_workers.enqueue(std::move(worker)); }
	inline void PushWorkers(std::vector<std::unique_ptr<WorkerBase>> &&workers) {
		m_workers.enqueue_bulk(std::make_move_iterator(workers.begin()), workers.size());
	}

	std::shared_ptr<Chunk> FindChunk(const ChunkPos3 &position) const;
	std::shared_ptr<Chunk> PushChunk(const ChunkPos3 &position);
	void EraseChunk(const ChunkPos3 &position) { m_chunks.erase(position); }

	void Update(const glm::vec3 &position);

	World() { launch_worker_threads(); }
	void Join(); // must be called in main thread
};

#endif
