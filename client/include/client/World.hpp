#ifndef HYPERCRAFT_CLIENT_WORLD_HPP
#define HYPERCRAFT_CLIENT_WORLD_HPP

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <blockingconcurrentqueue.h>

#include <atomic>
#include <thread>
#include <unordered_map>
#include <vector>

#include <client/Chunk.hpp>

#include <common/WorkPool.hpp>

namespace hc::client {

class WorldRenderer;
class ClientBase;

class World : public std::enable_shared_from_this<World> {
public:
	inline static std::shared_ptr<World> Create(const std::shared_ptr<WorkPool> &work_pool_ptr) {
		return std::make_shared<World>(work_pool_ptr);
	}

private:
	// Parent weak_ptrs
	std::weak_ptr<WorldRenderer> m_world_renderer_weak_ptr;
	friend class WorldRenderer;

	std::weak_ptr<ClientBase> m_client_weak_ptr;
	friend class LocalClient;
	friend class ENetClient;

	// Chunks
	std::unordered_map<ChunkPos3, std::shared_ptr<Chunk>> m_chunks;

	// Work Queue
	std::shared_ptr<WorkPool> m_work_pool_ptr;

public:
	inline explicit World(const std::shared_ptr<WorkPool> &work_pool_ptr) : m_work_pool_ptr{work_pool_ptr} {}
	~World();

	inline const std::weak_ptr<WorldRenderer> &GetWorldRendererWeakPtr() const { return m_world_renderer_weak_ptr; }
	inline std::shared_ptr<WorldRenderer> LockWorldRenderer() const { return m_world_renderer_weak_ptr.lock(); }

	inline const std::weak_ptr<ClientBase> &GetClientWeakPtr() const { return m_client_weak_ptr; }
	inline std::shared_ptr<ClientBase> LockClient() const { return m_client_weak_ptr.lock(); }

	inline const auto &GetWorkPoolPtr() const { return m_work_pool_ptr; }

	std::shared_ptr<Chunk> FindChunk(const ChunkPos3 &position) const;
	std::shared_ptr<Chunk> PushChunk(const ChunkPos3 &position);
	void EraseChunk(const ChunkPos3 &position) { m_chunks.erase(position); }

	void Update(const glm::vec3 &position);
};

} // namespace hc::client

#endif
