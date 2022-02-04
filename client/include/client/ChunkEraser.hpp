#ifndef CUBECRAFT3_CLIENT_CHUNK_ERASER_HPP
#define CUBECRAFT3_CLIENT_CHUNK_ERASER_HPP

#include <client/Chunk.hpp>
#include <client/WorkerBase.hpp>

class ChunkEraser : public WorkerBase {
private:
	std::shared_ptr<Chunk> m_chunk_ptr;

public:
	inline explicit ChunkEraser(const std::shared_ptr<Chunk> &chunk_ptr) : m_chunk_ptr(chunk_ptr) {}
	static inline std::unique_ptr<ChunkEraser> Create(const std::shared_ptr<Chunk> &chunk_ptr) {
		return std::make_unique<ChunkEraser>(chunk_ptr);
	}
	inline void Run() override {}
	~ChunkEraser() override = default;
};

#endif
