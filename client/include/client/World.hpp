#ifndef HYPERCRAFT_CLIENT_WORLD_HPP
#define HYPERCRAFT_CLIENT_WORLD_HPP

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/hash.hpp>
#include <glm/gtx/string_cast.hpp>

#include <blockingconcurrentqueue.h>
#include <cuckoohash_map.hh>

#include <atomic>
#include <thread>
#include <unordered_map>
#include <vector>

#include <client/Chunk.hpp>
#include <client/ChunkPool.hpp>
#include <client/ChunkTaskPool.hpp>

#include "Config.hpp"

namespace hc::client {

class WorldRenderer;
class ClientBase;

class World : public std::enable_shared_from_this<World> {
public:
	inline static std::shared_ptr<World> Create(ChunkPos1 load_chunk_radius, ChunkPos1 unload_chunk_radius) {
		return std::make_shared<World>(load_chunk_radius, unload_chunk_radius);
	}

private:
	// Parent weak_ptrs
	std::weak_ptr<WorldRenderer> m_world_renderer_weak_ptr;
	friend class WorldRenderer;

	std::weak_ptr<ClientBase> m_client_weak_ptr;
	friend class LocalClient;
	friend class ENetClient;

	friend class WorldWorker;
	friend class ChunkPool;
	friend class ChunkTaskPool;
	friend class ChunkUpdatePool;
	template <ChunkTaskType> friend class ChunkTaskRunner;

	std::atomic_uint64_t m_tick;
	static_assert(sizeof(ChunkPos3) <= sizeof(uint64_t));
	std::atomic_uint64_t m_center_chunk_pos;
	std::atomic<ChunkPos1> m_load_chunk_radius, m_unload_chunk_radius;

	// Chunks
	ChunkPool m_chunk_pool;
	ChunkTaskPool m_chunk_task_pool;

	void update();

public:
	inline explicit World(ChunkPos1 load_chunk_radius, ChunkPos1 unload_chunk_radius)
	    : m_chunk_pool{this}, m_chunk_task_pool{this}, m_load_chunk_radius{load_chunk_radius},
	      m_unload_chunk_radius{unload_chunk_radius} {}
	~World() = default;

	inline void Start() { update(); }

	inline void SetCenterPos(const glm::vec3 &pos) { SetCenterChunkPos(ChunkPosFromBlockPos(BlockPos3(pos))); }
	inline void SetCenterChunkPos(const ChunkPos3 &chunk_pos) {
		if (chunk_pos == GetCenterChunkPos())
			return;

		uint64_t u64 = 0;
		std::copy_n((const uint8_t *)(&chunk_pos), sizeof(ChunkPos3), (uint8_t *)(&u64));
		m_center_chunk_pos.store(u64, std::memory_order_release);

		update();
	}
	inline ChunkPos3 GetCenterChunkPos() const {
		uint64_t u64 = m_center_chunk_pos.load(std::memory_order_acquire);
		return *((const ChunkPos3 *)(&u64));
	}
	inline void SetLoadChunkRadius(ChunkPos1 radius) {
		radius = std::min(radius, (ChunkPos1)kWorldMaxLoadRadius);
		if (radius != GetLoadChunkRadius())
			return;
		m_load_chunk_radius.store(radius, std::memory_order_release);

		update();
	}
	inline ChunkPos1 GetLoadChunkRadius() const { return m_load_chunk_radius.load(std::memory_order_acquire); }

	inline void SetUnloadChunkRadius(ChunkPos1 radius) {
		if (radius != GetUnloadChunkRadius())
			return;
		m_unload_chunk_radius.store(radius, std::memory_order_release);

		update();
	}
	inline ChunkPos1 GetUnloadChunkRadius() const { return m_unload_chunk_radius.load(std::memory_order_acquire); }

	inline const std::weak_ptr<WorldRenderer> &GetWorldRendererWeakPtr() const { return m_world_renderer_weak_ptr; }
	inline std::shared_ptr<WorldRenderer> LockRenderer() const { return m_world_renderer_weak_ptr.lock(); }

	inline const std::weak_ptr<ClientBase> &GetClientWeakPtr() const { return m_client_weak_ptr; }
	inline std::shared_ptr<ClientBase> LockClient() const { return m_client_weak_ptr.lock(); }

	inline uint64_t GetCurrentTick() const { return m_tick.load(std::memory_order_acquire); }
	inline void NextTick() {
		m_tick.fetch_add(1, std::memory_order_acq_rel);
		m_chunk_task_pool.ProduceTickTasks();
	}

	const auto &GetChunkPool() const { return m_chunk_pool; }
	const auto &GetChunkTaskPool() const { return m_chunk_task_pool; }

	inline void SetBlock(const BlockPos3 &pos, block::Block block) {
		auto [chunk_pos, inner_pos] = ChunkInnerPosFromBlockPos(pos);
		auto entry = ChunkBlockEntry{.index = InnerIndex3FromPos(inner_pos), .block = block};
		m_chunk_task_pool.Push<ChunkTaskType::kSetBlock>(chunk_pos, std::span{&entry, 1}, ChunkUpdateType::kLocal);
	}
	inline std::optional<block::Block> GetBlock(const BlockPos3 &pos) const {
		auto [chunk_pos, inner_pos] = ChunkInnerPosFromBlockPos(pos);
		auto chunk = m_chunk_pool.FindChunk(chunk_pos);
		return chunk ? std::optional(chunk->GetBlock(inner_pos.x, inner_pos.y, inner_pos.z)) : std::nullopt;
	}
	inline void SetBlockBulk(std::span<const std::pair<BlockPos3, block::Block>> blocks) {
		std::unordered_map<ChunkPos3, std::vector<ChunkBlockEntry>> chunk_set_blocks;

		for (const auto &b : blocks) {
			auto [chunk_pos, inner_pos] = ChunkInnerPosFromBlockPos(b.first);
			chunk_set_blocks[chunk_pos].push_back(
			    {.index = (InnerIndex3)InnerIndex3FromPos(inner_pos), .block = b.second});
		}

		for (const auto &c : chunk_set_blocks)
			m_chunk_task_pool.Push<ChunkTaskType::kSetBlock>(c.first, std::span{c.second}, ChunkUpdateType::kLocal);
	}
};

} // namespace hc::client

#endif
