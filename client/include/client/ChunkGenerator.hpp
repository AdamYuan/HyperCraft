#ifndef HYPERCRAFT_CLIENT_CHUNK_GENERATOR_HPP
#define HYPERCRAFT_CLIENT_CHUNK_GENERATOR_HPP

#include <client/ChunkWorkerBase.hpp>

namespace hc::client {

class ChunkGenerator : public ChunkWorkerBase {
public:
	static inline std::unique_ptr<ChunkGenerator> Create(const std::weak_ptr<Chunk> &chunk_weak_ptr) {
		return std::make_unique<ChunkGenerator>(chunk_weak_ptr);
	}

	explicit ChunkGenerator(const std::weak_ptr<Chunk> &chunk_ptr) : ChunkWorkerBase(chunk_ptr) {}
	~ChunkGenerator() override = default;
	void Run() override;
};

} // namespace hc::client

#endif
