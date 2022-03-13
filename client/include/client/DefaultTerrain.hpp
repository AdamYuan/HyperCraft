#ifndef CUBECRAFT3_CLIENT_DEFAULT_TERRAIN_HPP
#define CUBECRAFT3_CLIENT_DEFAULT_TERRAIN_HPP

#include <FastNoise/FastNoise.h>
#include <client/TerrainBase.hpp>
#include <common/Biome.hpp>
#include <common/Position.hpp>
#include <common/Size.hpp>

#include <bitset>

#include <spdlog/spdlog.h>

class DefaultTerrain : public TerrainBase {
private:
	// Biome
	static constexpr uint32_t kBiomeMapSize = 4, kSampleScale = 1, kOceanSampleScale = 16, kHeightRange = 512;
	static constexpr Biome kBiomeMap[kBiomeMapSize][kBiomeMapSize] = {
	    // [precipitation][temperature]
	    {Biomes::kSnowMountain, Biomes::kSavanna, Biomes::kDesert, Biomes::kDesert},
	    {Biomes::kSnowMountain, Biomes::kSavanna, Biomes::kDesert, Biomes::kDesert},
	    {Biomes::kMountain, Biomes::kPlain, Biomes::kPlain, Biomes::kForest},
	    {Biomes::kBorealForest, Biomes::kForest, Biomes::kForest, Biomes::kRainForest}};

	inline static constexpr Biome biome_map_scaled(uint32_t x, uint32_t y) {
		x /= kSampleScale;
		y /= kSampleScale;
		return kBiomeMap[x >= kBiomeMapSize ? kBiomeMapSize - 1 : x][y >= kBiomeMapSize ? kBiomeMapSize - 1 : y];
	}
	inline static constexpr Biome biome_map_scaled(uint32_t x, uint32_t y, int32_t z) {
		return z <= 0 ? Biomes::kOcean : biome_map_scaled(x, y);
	}
	inline static constexpr float biome_prop_remap(float prop) {
		return std::clamp(prop, -0.5f, 0.5f) +
		       0.5f; // prop + 1.0f) * 0.5f; //((prop >= 0.0f ? std::sqrt(prop) : -std::sqrt(-prop)) + 1.0f) * 0.5f;
	}
	inline static constexpr float cubic(float x, float a, float b, float c) {
		return x * x * x * a + x * x * b + x * c;
	}
	inline static constexpr float biome_height_transform(Biome biome, float height) {
		switch (biome) {
		case Biomes::kOcean:
			return std::min(height * 0.25f, 0.0f);
		case Biomes::kPlain:
			return cubic(height, -0.05f, 0.0f, 0.15f) + 0.005f;
		case Biomes::kSavanna:
			return cubic(height, -0.05f, 0.0f, 0.2f) + 0.005f;
		case Biomes::kMountain:
			return cubic(height, -3.2f, 4.3, 1.0f);
		case Biomes::kSnowMountain:
			return cubic(height, -4.0f, 6.0, 0.1f);
		case Biomes::kDesert:
			return cubic(height, 1.2f, -0.7f, 0.2f) + 0.01f;
		case Biomes::kForest:
		case Biomes::kRainForest:
		case Biomes::kBorealForest:
			return cubic(height, -0.8f, 1.3f, -0.1f) + 0.005f;
		default:
			return 0.0f;
		}
	}
	inline static constexpr Biome get_biome(float precipitation, float temperature, float height) {
		if (height <= 0.0f)
			return Biomes::kOcean;
		float x = biome_prop_remap(precipitation) * kBiomeMapSize * kSampleScale,
		      y = biome_prop_remap(temperature) * kBiomeMapSize * kSampleScale;
		auto ix = (uint32_t)x, iy = (uint32_t)y;

		return biome_map_scaled(ix, iy);

		/*if (height < 0.0f)
		    return Biomes::kOcean;
		auto x = uint32_t(biome_prop_remap(precipitation) * kBiomeMapSize),
		     y = uint32_t(biome_prop_remap(temperature) * kBiomeMapSize);
		x = x >= kBiomeMapSize ? kBiomeMapSize - 1 : x;
		y = y >= kBiomeMapSize ? kBiomeMapSize - 1 : y;
		return kBiomeMap[x][y];*/
	}
	inline static constexpr int32_t get_height(float precipitation, float temperature, float height) {
		float x = biome_prop_remap(precipitation) * kBiomeMapSize * kSampleScale,
		      y = biome_prop_remap(temperature) * kBiomeMapSize * kSampleScale, z = height * kOceanSampleScale;
		auto ix = (uint32_t)x, iy = (uint32_t)y;
		int32_t iz = z >= 0 ? (int32_t)z : (int32_t)z - 1;
		float tx = x - float(ix), ty = y - float(iy), tz = z - float(iz);

		float b000 = biome_height_transform(biome_map_scaled(ix, iy, iz), height),
		      b100 = biome_height_transform(biome_map_scaled(ix + 1, iy, iz), height),
		      b010 = biome_height_transform(biome_map_scaled(ix, iy + 1, iz), height),
		      b110 = biome_height_transform(biome_map_scaled(ix + 1, iy + 1, iz), height),
		      b001 = biome_height_transform(biome_map_scaled(ix, iy, iz + 1), height),
		      b101 = biome_height_transform(biome_map_scaled(ix + 1, iy, iz + 1), height),
		      b011 = biome_height_transform(biome_map_scaled(ix, iy + 1, iz + 1), height),
		      b111 = biome_height_transform(biome_map_scaled(ix + 1, iy + 1, iz + 1), height);

#define LERP(a, b, t) (a + t * (b - a))
		return (int32_t)std::ceil(LERP(LERP(LERP(b000, b100, tx), LERP(b010, b110, tx), ty),
		                               LERP(LERP(b001, b101, tx), LERP(b011, b111, tx), ty), tz) *
		                          (float)kHeightRange);
#undef LERP
	}

	// Noise generators
	static constexpr float kBiomeNoiseFrequency = 0.002f, kHeightNoiseFrequency = 0.001f;
	FastNoise::SmartNode<FastNoise::Perlin> m_biome_precipitation_noise;
	FastNoise::SmartNode<FastNoise::SeedOffset> m_biome_temperature_noise;

	FastNoise::SmartNode<FastNoise::FractalFBm> m_biome_bias_x;
	FastNoise::SmartNode<FastNoise::FractalFBm> m_biome_bias_y;
	FastNoise::SmartNode<FastNoise::Remap> m_biome_bias_remap_x;
	FastNoise::SmartNode<FastNoise::Remap> m_biome_bias_remap_y;

	/* FastNoise::SmartNode<FastNoise::CellularLookup> m_biome_precipitation_cellular_lookup;
	FastNoise::SmartNode<FastNoise::CellularLookup> m_biome_temperature_cellular_lookup; */

	/*FastNoise::SmartNode<FastNoise::Remap> m_biome_precipitation_remap;
	FastNoise::SmartNode<FastNoise::Remap> m_biome_temperature_remap;*/

	FastNoise::SmartNode<FastNoise::DomainOffset> m_biome_precipitation;
	FastNoise::SmartNode<FastNoise::DomainOffset> m_biome_temperature;

	FastNoise::SmartNode<FastNoise::GeneratorCache> m_biome_precipitation_cache;
	FastNoise::SmartNode<FastNoise::GeneratorCache> m_biome_temperature_cache;

	FastNoise::SmartNode<> m_cave_noise;

	FastNoise::SmartNode<FastNoise::FractalFBm> m_height_noise;
	FastNoise::SmartNode<FastNoise::GeneratorCache> m_height_cache;

	FastNoise::SmartNode<FastNoise::White> m_meta_noise;
	FastNoise::SmartNode<FastNoise::GeneratorCache> m_meta_cache;

	void initialize_biome_noise();
	void initialize_height_noise();
	void initialize_cave_noise();
	void initialize_meta_noise();

	struct XZInfo {
		int32_t height_map[kChunkSize * kChunkSize], max_height;
		float meta[kChunkSize * kChunkSize];

		Biome biome_map[kChunkSize * kChunkSize];
	};
	TerrainCache<ChunkPos2, XZInfo, 30 * 30 * 30> m_xz_cache;
	void generate_xz_info(const ChunkPos2 &pos, XZInfo *info);

public:
	inline explicit DefaultTerrain(uint32_t seed)
	    : TerrainBase(seed), m_xz_cache{[this](const ChunkPos2 &pos, XZInfo *info) { generate_xz_info(pos, info); }} {
		initialize_biome_noise();
		initialize_height_noise();
		initialize_cave_noise();
		initialize_meta_noise();
	}
	~DefaultTerrain() override = default;
	inline static std::unique_ptr<TerrainBase> Create(uint32_t seed) { return std::make_unique<DefaultTerrain>(seed); }
	void Generate(const std::shared_ptr<Chunk> &chunk_ptr, uint32_t *y_peak) override;
};

#endif
