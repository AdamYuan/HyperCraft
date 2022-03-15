#ifndef CUBECRAFT3_CLIENT_DEFAULT_TERRAIN_HPP
#define CUBECRAFT3_CLIENT_DEFAULT_TERRAIN_HPP

#include <FastNoise/FastNoise.h>
#include <client/TerrainBase.hpp>
#include <common/Biome.hpp>
#include <common/Block.hpp>
#include <common/Position.hpp>
#include <common/Size.hpp>

#include <bitset>

#include <spdlog/spdlog.h>

class DefaultTerrain : public TerrainBase {
private:
	// Biome
	static constexpr uint32_t kBiomeMapSize = 4, kSampleScale = 1, kOceanSampleScale = 16, kHeightRange = 256;
	static constexpr Biome kBiomeMap[kBiomeMapSize][kBiomeMapSize] = {
	    // [precipitation][temperature]
	    {Biomes::kSnowMountain, Biomes::kSavanna, Biomes::kDesert, Biomes::kDesert},
	    {Biomes::kSnowMountain, Biomes::kSavanna, Biomes::kDesert, Biomes::kDesert},
	    {Biomes::kMountain, Biomes::kPlain, Biomes::kPlain, Biomes::kForest},
	    {Biomes::kBorealForest, Biomes::kForest, Biomes::kForest, Biomes::kTropicalForest}};

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
			return cubic(height, -2.0f, 3.0, 0.0f) + 0.005f;
		case Biomes::kSnowMountain:
			return cubic(height, -2.2f, 3.2, 0.3f) + 0.005f;
		case Biomes::kDesert:
			return cubic(height, 1.2f, -0.7f, 0.2f) + 0.01f;
		case Biomes::kForest:
		case Biomes::kTropicalForest:
		case Biomes::kBorealForest:
			return cubic(height, -0.8f, 1.5f, 0.0f) + 0.005f;
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
		float fh = LERP(LERP(LERP(b000, b100, tx), LERP(b010, b110, tx), ty),
		                LERP(LERP(b001, b101, tx), LERP(b011, b111, tx), ty), tz) *
		           (float)kHeightRange;
		return fh >= 0 ? (int32_t)fh + 1 : (int32_t)fh;
#undef LERP
	}

	// Noise generators
	static constexpr float kBiomeNoiseFrequency = 0.002f, kBiomeCellLookupFrequency = 0.1f,
	                       kHeightNoiseFrequency = 0.001f, kCaveNoiseFrequency = 0.016f;
	FastNoise::SmartNode<FastNoise::Perlin> m_biome_precipitation_noise;
	FastNoise::SmartNode<FastNoise::SeedOffset> m_biome_temperature_noise;
	FastNoise::SmartNode<FastNoise::Remap> m_biome_bias_x_remap, m_biome_bias_y_remap;
	FastNoise::SmartNode<FastNoise::FractalFBm> m_biome_bias_x, m_biome_bias_y;
	FastNoise::SmartNode<FastNoise::CellularLookup> m_biome_precipitation_cell_lookup, m_biome_temperature_cell_lookup;
	FastNoise::SmartNode<FastNoise::DomainOffset> m_biome_precipitation_cell_offset, m_biome_temperature_cell_offset,
	    m_biome_precipitation_offset, m_biome_temperature_offset;
	FastNoise::SmartNode<FastNoise::GeneratorCache> m_biome_precipitation_cell_cache, m_biome_temperature_cell_cache,
	    m_biome_precipitation_cache, m_biome_temperature_cache;

	FastNoise::SmartNode<> m_cave_noise;

	FastNoise::SmartNode<FastNoise::FractalFBm> m_height_noise;
	FastNoise::SmartNode<FastNoise::GeneratorCache> m_height_cache;

	FastNoise::SmartNode<FastNoise::White> m_meta_noise;
	FastNoise::SmartNode<FastNoise::GeneratorCache> m_meta_cache;

	void initialize_biome_noise();
	void initialize_height_noise();
	void initialize_cave_noise();
	void initialize_meta_noise();

	class DecorationInfo {
	private:
		std::unordered_map<glm::i32vec3, Block> m_blocks;
		int32_t m_y_min{INT32_MAX}, m_y_max{INT32_MIN};
		inline constexpr static bool block_cover(Block old, Block cur) {
			return old.GetTransparent() == cur.GetTransparent()
			           ? (old.GetID() == cur.GetID() ? cur.GetMeta() > old.GetMeta() : cur.GetID() > old.GetID())
			           : cur.GetTransparent() > old.GetTransparent();
		}

	public:
		// x, y, z indicates a position inside a ChunkSize x inf x ChunkSize vertical space
		inline void SetBlock(uint32_t x, int32_t y, uint32_t z, Block b) {
			const glm::i32vec3 key = {(int32_t)x, y, (int32_t)z};
			auto it = m_blocks.find(key);
			if (it == m_blocks.end())
				m_blocks[key] = b;
			else if (block_cover(it->second, b))
				it->second = b;
			m_y_min = std::min(m_y_min, y);
			m_y_max = std::max(m_y_max, y);
		}
		inline void Merge(const DecorationInfo &r) {
			m_y_min = std::min(m_y_min, r.m_y_min);
			m_y_max = std::max(m_y_max, r.m_y_max);
			for (const auto &i : r.m_blocks) {
				auto it = m_blocks.find(i.first);
				if (it == m_blocks.end())
					m_blocks[i.first] = i.second;
				else if (block_cover(it->second, i.second))
					it->second = i.second;
			}
		}
		void PopToChunk(const std::shared_ptr<Chunk> &chunk_ptr) const;
	};
	class DecorationGroup {
	private:
		DecorationInfo m_decorations[9];

	public:
		const DecorationInfo &GetInfo(uint32_t index) const { return m_decorations[index]; }
		inline void SetBlock(int32_t x, int32_t y, int32_t z, Block b) {
			int32_t cmp_x = x < 0 ? -1 : (x >= kChunkSize ? 1 : 0), cmp_z = z < 0 ? -1 : (z >= kChunkSize ? 1 : 0);
			x -= cmp_x * (int32_t)kChunkSize;
			z -= cmp_z * (int32_t)kChunkSize;
			if (x < 0 || x >= kChunkSize || z < 0 || z >= kChunkSize)
				return;
			m_decorations[cmp_xz_to_neighbour_index(cmp_x, cmp_z)].SetBlock(x, y, z, b);
		}
	};
	struct XZInfo {
		DecorationGroup decorations;
		int32_t height_map[kChunkSize * kChunkSize]{}, max_height{INT32_MIN};
		float meta[kChunkSize * kChunkSize]{};
		Biome biome_map[kChunkSize * kChunkSize]{};
		std::bitset<kChunkSize * kChunkSize> is_ground;
	};
	struct CombinedXZInfo {
		DecorationInfo decoration;
	};

	template <typename T, typename = std::enable_if_t<std::is_integral_v<T>>>
	static inline constexpr uint32_t cmp_xz_to_neighbour_index(T cmp_x, T cmp_z) {
		constexpr uint32_t kLookUp[3] = {1, 2, 0};
		return kLookUp[cmp_x + 1] * 3u + kLookUp[cmp_z + 1];
	}
	template <typename T, typename = std::enable_if_t<std::is_signed_v<T> && std::is_integral_v<T>>>
	static inline constexpr void neighbour_index_to_cmp_xz(uint32_t idx, T *cmp_xz) {
		constexpr T kRevLookUp[3] = {1, -1, 0};
		cmp_xz[1] = kRevLookUp[idx % 3u];
		cmp_xz[0] = kRevLookUp[idx / 3u];
	}
	static inline constexpr uint32_t opposite_xz_neighbour_index(uint32_t index) {
		constexpr uint32_t kOppositeLookUp[9] = {4, 3, 5, 1, 0, 2, 7, 6, 8};
		return kOppositeLookUp[index];
	}

	static constexpr uint32_t kXZCacheSize = 32768;
	TerrainCache<ChunkPos2, XZInfo, kXZCacheSize> m_xz_cache;
	TerrainCache<ChunkPos2, CombinedXZInfo, kXZCacheSize> m_combined_xz_cache;
	void generate_xz_info(const ChunkPos2 &pos, XZInfo *info);
	void generate_combined_xz_info(const ChunkPos2 &pos, const std::shared_ptr<const XZInfo> &center_xz_info,
	                               CombinedXZInfo *combined_info);

public:
	inline explicit DefaultTerrain(uint32_t seed) : TerrainBase(seed) {
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
