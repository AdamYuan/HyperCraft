#ifndef CHUNK_WORKER_HPP
#define CHUNK_WORKER_HPP

#include <utility>

#include "Chunk.hpp"

class ChunkWorker {
private:
	std::weak_ptr<Chunk> m_weak_chunk;

protected:
	std::shared_ptr<Chunk> m_chunk;
	virtual inline bool lock() { return (m_chunk = m_weak_chunk.lock()) != nullptr; };

public:
	virtual void Run() = 0;
	inline explicit ChunkWorker(const std::weak_ptr<Chunk> &weak_chunk) : m_weak_chunk(weak_chunk) {}
};

class ChunkWorkerS26 : public ChunkWorker {
protected:
	std::shared_ptr<Chunk> m_neighbour_chunks[26];
	inline bool lock() override {
		if (!ChunkWorker::lock())
			return false;
		for (uint32_t i = 0; i < 26; ++i)
			if ((m_neighbour_chunks[i] = m_chunk->LockNeighbour(i)) == nullptr)
				return false;
		return true;
	}
	template <typename T>
	inline typename std::enable_if<std::is_integral<T>::value, Block>::type get_block(T x, T y, T z) const {
		uint32_t nei_idx = Chunk::GetBlockNeighbourIndex(x, y, z);
		return nei_idx == 26 ? m_chunk->GetBlock(x, y, z) : m_neighbour_chunks[nei_idx]->GetBlockFromNeighbour(x, y, z);
	}
	template <typename T>
	inline typename std::enable_if<std::is_integral<T>::value, Light>::type get_light(T x, T y, T z) const {
		uint32_t nei_idx = Chunk::GetBlockNeighbourIndex(x, y, z);
		return nei_idx == 26 ? m_chunk->GetLight(x, y, z) : m_neighbour_chunks[nei_idx]->GetLightFromNeighbour(x, y, z);
	}

	// TODO: Implement get_block_ref, get_light_ref, set_block, set_light

public:
	explicit ChunkWorkerS26(const std::weak_ptr<Chunk> &weak_chunk) : ChunkWorker(weak_chunk) {}
};

// TODO: Implement this (only 6 neighbours (6 faces of a cube))
class ChunkWorkerS6 : public ChunkWorker {};

#endif
