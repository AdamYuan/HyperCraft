#ifndef CUBECRAFT3_CLIENT_CHUNK_WORKER_HPP
#define CUBECRAFT3_CLIENT_CHUNK_WORKER_HPP

#include <client/Chunk.hpp>
#include <client/WorkerBase.hpp>

class ChunkWorkerBase : public WorkerBase {
private:
	std::weak_ptr<Chunk> m_chunk_weak_ptr;

protected:
	std::shared_ptr<Chunk> m_chunk_ptr;
	virtual inline bool lock() { return (m_chunk_ptr = m_chunk_weak_ptr.lock()) != nullptr; }
	void push_worker(std::unique_ptr<ChunkWorkerBase> &&worker) const;

public:
	inline explicit ChunkWorkerBase(const std::weak_ptr<Chunk> &chunk_weak_ptr) : m_chunk_weak_ptr(chunk_weak_ptr) {}
	~ChunkWorkerBase() override = default;
};

class ChunkWorkerS26Base : public ChunkWorkerBase {
protected:
	std::shared_ptr<Chunk> m_neighbour_chunk_ptr[26];
	inline bool lock() override {
		if (!ChunkWorkerBase::lock())
			return false;
		for (uint32_t i = 0; i < 26; ++i)
			if ((m_neighbour_chunk_ptr[i] = m_chunk_ptr->LockNeighbour(i)) == nullptr)
				return false;
		return true;
	}
	template <typename T>
	inline typename std::enable_if<std::is_integral<T>::value, Block>::type get_block(T x, T y, T z) const {
		uint32_t nei_idx = Chunk::GetBlockNeighbourIndex(x, y, z);
		return nei_idx == 26 ? m_chunk_ptr->GetBlock(x, y, z)
		                     : m_neighbour_chunk_ptr[nei_idx]->GetBlockFromNeighbour(x, y, z);
	}
	template <typename T>
	inline typename std::enable_if<std::is_integral<T>::value, Light>::type get_light(T x, T y, T z) const {
		uint32_t nei_idx = Chunk::GetBlockNeighbourIndex(x, y, z);
		return nei_idx == 26 ? m_chunk_ptr->GetLight(x, y, z)
		                     : m_neighbour_chunk_ptr[nei_idx]->GetLightFromNeighbour(x, y, z);
	}

	// TODO: Implement get_block_ref, get_light_ref, set_block, set_light

public:
	explicit ChunkWorkerS26Base(const std::weak_ptr<Chunk> &chunk_ptr) : ChunkWorkerBase(chunk_ptr) {}
	~ChunkWorkerS26Base() override = default;
};

// TODO: Implement this (only 6 neighbours (6 faces of a cube), for lighting or fluid update)
class ChunkWorkerS6Base : public ChunkWorkerBase {

	~ChunkWorkerS6Base() override = default;
};

#endif
