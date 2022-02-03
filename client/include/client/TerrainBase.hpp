#ifndef CUBECRAFT3_CLIENT_TERRAIN_BASE_HPP
#define CUBECRAFT3_CLIENT_TERRAIN_BASE_HPP

#include <cinttypes>
#include <memory>

class Chunk;
class TerrainBase {
private:
	uint32_t m_seed{};

public:
	inline explicit TerrainBase(uint32_t seed) : m_seed{seed} {}
	virtual ~TerrainBase() = default;
	inline uint32_t GetSeed() const { return m_seed; }

	// chunk: the chunk to be generated; peak: the estimated block peak of the y-axis (used to generate sunlight)
	virtual void Generate(const std::shared_ptr<Chunk> &chunk_ptr, uint32_t *y_peak) = 0;
};

#endif
