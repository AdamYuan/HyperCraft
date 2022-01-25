#ifndef CUBECRAFT3_CLIENT_CHUNK_GENERATOR_HPP
#define CUBECRAFT3_CLIENT_CHUNK_GENERATOR_HPP

#include <client/ChunkWorker.hpp>

class ChunkGenerator : public ChunkWorker {
public:
	static inline std::unique_ptr<ChunkWorker> Create(const std::weak_ptr<Chunk> &chunk_ptr) {
		return std::make_unique<ChunkGenerator>(chunk_ptr);
	}

	explicit ChunkGenerator(const std::weak_ptr<Chunk> &chunk_ptr) : ChunkWorker(chunk_ptr) {}
	void Run() override;
};

#endif
