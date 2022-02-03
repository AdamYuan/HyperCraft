#ifndef CUBECRAFT3_CLIENT_DEFAULT_TERRAIN_HPP
#define CUBECRAFT3_CLIENT_DEFAULT_TERRAIN_HPP

#include <client/TerrainBase.hpp>

class DefaultTerrain : public TerrainBase {
public:
	inline explicit DefaultTerrain(uint32_t seed) : TerrainBase(seed) {}
	~DefaultTerrain() override = default;
	inline static std::unique_ptr<TerrainBase> Create(uint32_t seed) { return std::make_unique<DefaultTerrain>(seed); }
	void Generate(const std::shared_ptr<Chunk> &chunk_ptr, uint32_t *y_peak) override;
};

#endif
