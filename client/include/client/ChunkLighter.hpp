#ifndef CUBECRAFT3_CLIENT_CHUNK_LIGHTER_HPP
#define CUBECRAFT3_CLIENT_CHUNK_LIGHTER_HPP

#include <client/ChunkWorkerBase.hpp>
#include <common/Light.hpp>

class ChunkLighter : public ChunkWorkerS26Base {
private:
	bool m_initial_pass{false};
	std::vector<glm::ivec3> m_mods;

public:
	static inline std::unique_ptr<ChunkLighter> CreateInitial(const std::shared_ptr<Chunk> &chunk_ptr) {
		return std::make_unique<ChunkLighter>(chunk_ptr);
	}
	static inline std::unique_ptr<ChunkLighter> Create(const std::shared_ptr<Chunk> &chunk_ptr, bool initial_pass,
	                                                   std::vector<glm::ivec3> &&mods) {
		return std::make_unique<ChunkLighter>(chunk_ptr, initial_pass, std::move(mods));
	}

	explicit ChunkLighter(const std::shared_ptr<Chunk> &chunk_ptr)
	    : ChunkWorkerS26Base(chunk_ptr), m_initial_pass{true} {
		chunk_ptr->PendLightVersion();
	}
	explicit ChunkLighter(const std::shared_ptr<Chunk> &chunk_ptr, std::vector<glm::ivec3> &&mods)
	    : ChunkWorkerS26Base(chunk_ptr), m_initial_pass{false}, m_mods(std::move(mods)) {
		chunk_ptr->PendLightVersion();
	}
	explicit ChunkLighter(const std::shared_ptr<Chunk> &chunk_ptr, bool initial_pass, std::vector<glm::ivec3> &&mods)
	    : ChunkWorkerS26Base(chunk_ptr), m_initial_pass{initial_pass}, m_mods(std::move(mods)) {
		chunk_ptr->PendLightVersion();
	}
	~ChunkLighter() override = default;
	void Run() override;
};

#endif
