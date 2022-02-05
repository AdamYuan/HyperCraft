#ifndef CUBECRAFT3_CLIENT_CHUNK_ERASER_HPP
#define CUBECRAFT3_CLIENT_CHUNK_ERASER_HPP

#include <client/Chunk.hpp>
#include <client/WorkerBase.hpp>
#include <utility>

class ChunkEraser : public WorkerBase {
private:
	std::shared_ptr<Chunk> m_chunk_ptr;

public:
	inline explicit ChunkEraser(std::shared_ptr<Chunk> &&chunk_ptr) : m_chunk_ptr(std::move(chunk_ptr)) {}
	static inline std::unique_ptr<ChunkEraser> Create(std::shared_ptr<Chunk> &&chunk_ptr) {
		return std::make_unique<ChunkEraser>(std::move(chunk_ptr));
	}
	inline void Run() override {}
	~ChunkEraser() override = default;
};

#endif
