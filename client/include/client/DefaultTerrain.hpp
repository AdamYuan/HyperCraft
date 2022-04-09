#ifndef CUBECRAFT3_CLIENT_DEFAULT_TERRAIN_HPP
#define CUBECRAFT3_CLIENT_DEFAULT_TERRAIN_HPP

#include <client/TerrainBase.hpp>
#include <common/Biome.hpp>
#include <common/Block.hpp>
#include <common/Position.hpp>
#include <common/Size.hpp>

#include <bitset>
#include <type_traits>

#include <FastNoise/FastNoise.h>

#include <spdlog/spdlog.h>

class DefaultTerrain : public TerrainBase {
private:
	// Biome
	static constexpr uint32_t kBiomeMapSize = 4, kSampleScale = 1, kOceanSampleScale = 16, kHeightRange = 256;
	static constexpr Biome kBiomeMap[kBiomeMapSize][kBiomeMapSize] = {
	    // [precipitation][temperature]
	    {Biomes::kGlacier, Biomes::kTundra, Biomes::kDesert, Biomes::kDesert},
	    {Biomes::kGlacier, Biomes::kSavanna, Biomes::kSavanna, Biomes::kDesert},
	    {Biomes::kTundra, Biomes::kPlain, Biomes::kPlain, Biomes::kForest},
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
			return std::min(height * 0.5f, 0.0f);
		case Biomes::kPlain:
			return cubic(height, -0.05f, 0.0f, 0.15f) + 0.005f;
		case Biomes::kSavanna:
			return cubic(height, -0.05f, 0.0f, 0.2f) + 0.005f;
		case Biomes::kTundra:
			return cubic(height, -2.0f, 1.0f, 2.5f) + 0.005f;
		case Biomes::kGlacier:
			return cubic(height, -2.0f, 1.0f, 3.0f) + 0.005f;
		case Biomes::kDesert:
			return cubic(height, 1.2f, -0.7f, 0.2f) + 0.01f;
		case Biomes::kForest:
			return cubic(height, -0.8f, 1.5f, 0.0f) + 0.005f;
		case Biomes::kTropicalForest:
			return cubic(height, -0.8f, 1.5f, 0.5f) + 0.005f;
		case Biomes::kBorealForest:
			return cubic(height, -0.8f, 1.5f, 0.6f) + 0.005f;
		default:
			return 0.0f;
		}
	}
	inline static constexpr Biome get_biome(float precipitation, float temperature) {
		float x = biome_prop_remap(precipitation) * kBiomeMapSize * kSampleScale + 0.5f,
		      y = biome_prop_remap(temperature) * kBiomeMapSize * kSampleScale + 0.5f;
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
	static constexpr float kBiomeNoiseFrequency = 0.005f, kBiomeCellLookupFrequency = 0.1f,
	                       kHeightNoiseFrequency = 0.001f, kCaveNoiseFrequency = 0.005f;

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
	FastNoise::SmartNode<FastNoise::GeneratorCache> m_height_noise_cache;

	void initialize_biome_noise();
	void initialize_height_noise();
	void initialize_cave_noise();

	class DecorationInfo {
	private:
		std::unordered_map<glm::i32vec3, Block> m_blocks;
		int32_t m_y_min{INT32_MAX}, m_y_max{INT32_MIN};
		inline constexpr static bool block_cover(Block old, Block cur) {
			return old.GetTransparent() == cur.GetTransparent()
			           ? (old.GetID() == cur.GetID() ? cur.GetMeta() > old.GetMeta() : cur.GetID() > old.GetID())
			           : cur.GetTransparent() < old.GetTransparent();
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
		inline void PopToLightMap(int32_t light_map[kChunkSize * kChunkSize]) const {
			for (const auto &i : m_blocks) {
				if (!i.second.GetDirectLightPass()) {
					uint32_t idx = i.first.z * kChunkSize + i.first.x;
					light_map[idx] = std::max(light_map[idx], i.first.y);
				}
			}
		}
		void PopToChunk(const std::shared_ptr<Chunk> &chunk_ptr) const;
	};
	class DecorationGroup {
	private:
		DecorationInfo m_decorations[9];

		inline void set_block_line(int32_t sx, int32_t sy, int32_t sz, int32_t dx, int32_t dy, int32_t dz, Block b) {
			if (dx == 0 && dy == 0 && dz == 0)
				return;
			int32_t lb{0}, rb{0};
			float co;
			uint32_t adx = std::abs(dx), ady = std::abs(dy), adz = std::abs(dz);
			if (adx > ady && adx > adz) {
				// x axis
				co = 1.0f / float(dx);
				(dx > 0 ? rb : lb) = dx;
				for (int32_t x = lb; x <= rb; ++x) {
					int32_t y = sy + int32_t(float(x) * co * float(dy));
					int32_t z = sz + int32_t(float(x) * co * float(dz));
					SetBlock(x + sx, y, z, b);
				}
			} else if (ady > adz) {
				// y axis
				co = 1.0f / float(dy);
				(dy > 0 ? rb : lb) = dy;
				for (int32_t y = lb; y <= rb; ++y) {
					int32_t x = sx + int32_t(float(y) * co * float(dx));
					int32_t z = sz + int32_t(float(y) * co * float(dz));
					SetBlock(x, y + sy, z, b);
				}
			} else {
				// z axis
				co = 1.0f / float(dz);
				(dz > 0 ? rb : lb) = dz;
				for (int32_t z = lb; z <= rb; ++z) {
					int32_t x = sx + int32_t(float(z) * co * float(dx));
					int32_t y = sy + int32_t(float(z) * co * float(dy));
					SetBlock(x, y, z + sz, b);
				}
			}
		}

		inline void set_block_disc(int32_t sx, int32_t sy, int32_t sz, int32_t r, Block b) {
			for (int32_t cx = -r; cx <= r; ++cx)
				for (int32_t cz = -r; cz <= r; ++cz) {
					if (std::abs(cx) == r && std::abs(cz) == r)
						continue;
					SetBlock(sx + cx, sy, sz + cz, b);
				}
		}

		inline void set_block_cluster(int32_t sx, int32_t sy, int32_t sz, int32_t r, Block b) {
			for (int32_t cx = -r; cx <= r; ++cx)
				for (int32_t cy = -r; cy <= r; ++cy)
					for (int32_t cz = -r; cz <= r; ++cz) {
						if ((std::abs(cx) == r) + (std::abs(cy) == r) + (std::abs(cz) == r) >= 2)
							continue;
						SetBlock(sx + cx, sy + cy, sz + cz, b);
					}
		}

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

		template <typename RNG>
		inline auto GenJungleTree(RNG &rng, int32_t x, int32_t y, int32_t z) -> decltype(rng() - 1, void()) {
			// trunk
			int32_t trunk_height = rng() % 22 + 8;
			for (int32_t i = 0; i < trunk_height; ++i)
				SetBlock(x, i + y, z, {Blocks::kLog, BlockMetas::Tree::kJungle});

			// root
			int32_t root_height = trunk_height / 5 - rng() % 2, root_radius = root_height;
			for (uint32_t b = 0; b < 4; ++b) {
				float a = float(rng() % 20) / 10 * 3.14;
				glm::i32vec3 d = {int32_t((float)root_radius * std::cos(a)), -int32_t(rng() % 2) - root_height - 3,
				                  int32_t((float)root_radius * std::sin(a))};
				if (d.x || d.z)
					set_block_line(x, y + root_height, z, d.x, d.y, d.z, {Blocks::kLog, BlockMetas::Tree::kJungle});
				else
					break;
			}

			// crown
			int32_t crown_thickness = std::clamp(trunk_height / 7, 2, 4) + rng() % 2,
			        crown_radius = std::max(crown_thickness - (int32_t)(rng() % 2), 2);
			set_block_disc(x, y + trunk_height, z, crown_radius / 2, {Blocks::kLeaves, BlockMetas::Tree::kJungle});
			if (crown_radius > 2) {
				set_block_disc(x, y + trunk_height - 1, z, 4 * crown_radius / 5,
				               {Blocks::kLeaves, BlockMetas::Tree::kJungle});
				for (int32_t i = 2; i < crown_thickness; ++i) {
					set_block_disc(x, y + trunk_height - i, z, crown_radius,
					               {Blocks::kLeaves, BlockMetas::Tree::kJungle});
				}
			} else {
				for (int32_t i = 1; i < crown_thickness; ++i) {
					set_block_disc(x, y + trunk_height - i, z, crown_radius,
					               {Blocks::kLeaves, BlockMetas::Tree::kJungle});
				}
			}

			// branches
			int32_t branch_range = 2 * trunk_height / 3 + rng() % 3;
			for (int32_t i = 1; i <= branch_range; ++i) {
				if (rng() % 2 && rng() % (branch_range + 1) >= i) {
					int32_t bry = y + trunk_height - crown_thickness / 2 - i;
					glm::i32vec3 d = {rng() % 9 - 4, rng() % 5 - 1, rng() % 9 - 4};
					set_block_line(x, bry, z, d.x, d.y, d.z, {Blocks::kLog, BlockMetas::Tree::kJungle});
					int32_t r = i < branch_range / 2 ? rng() % 2 + 1 : 1;
					set_block_cluster(x + d.x, bry + d.y, z + d.z, r, {Blocks::kLeaves, BlockMetas::Tree::kJungle});
				}
			}
		}

		template <typename RNG>
		inline auto GenSpruceTree(RNG &rng, int32_t x, int32_t y, int32_t z) -> decltype(rng() - 1, void()) {
			int32_t trunk_height = rng() % 10 + 12;
			for (int32_t i = 0; i < trunk_height; ++i)
				SetBlock(x, i + y, z, {Blocks::kLog, BlockMetas::Tree::kSpruce});

			int32_t branch_range = 2 * trunk_height / 3 + rng() % 3;
			int32_t bottom_radius = std::min(branch_range / 3 + int32_t(rng() % 2), 3);
			float coef = float(bottom_radius) / float(branch_range);
			for (int32_t i = 0; i < branch_range; ++i) {
				auto r = (int32_t)std::ceil((float)i * coef);
				if (r == 0)
					continue;
				set_block_disc(x, y + trunk_height - i - 1, z, rng() % 16 >= i ? r : 2 * r / 3,
				               {Blocks::kLeaves, BlockMetas::Tree::kSpruce});

				for (uint32_t b = 0; b < 2; ++b) {
					float a = float(rng() % 20) / 10 * 3.14;
					glm::i32vec3 d = {int32_t((float)r * std::cos(a)), -r / 2, int32_t((float)r * std::sin(a))};
					if (d.x || d.z)
						set_block_line(x, y + trunk_height - i - 1, z, d.x, d.y, d.z,
						               {Blocks::kLog, BlockMetas::Tree::kSpruce});
					else
						break;
				}
			}
		}

		template <typename RNG>
		inline auto GenOakTree(RNG &rng, int32_t x, int32_t y, int32_t z) -> decltype(rng() - 1, void()) {
			int32_t trunk_height = rng() % 5 + 5;
			int32_t main_crown_size = rng() % 3 + 1;
			for (int32_t i = 0; i < trunk_height + main_crown_size; ++i)
				SetBlock(x, i + y, z, {Blocks::kLog, BlockMetas::Tree::kOak});
			set_block_cluster(x, y + trunk_height, z, main_crown_size, {Blocks::kLeaves, BlockMetas::Tree::kOak});

			uint32_t branch_count = rng() % 3 + 4;
			for (uint32_t i = 0; i < branch_count; ++i) {
				int32_t branch_height = std::max(trunk_height - 4 + (int32_t)(rng() % 6), 4), bry = y + branch_height;

				glm::i32vec3 d = {rng() % 5 - 2, rng() % 4 + 1, rng() % 5 - 2};
				set_block_line(x, bry, z, d.x, d.y, d.z, {Blocks::kLog, BlockMetas::Tree::kOak});
				set_block_cluster(x + d.x, bry + d.y, z + d.z, rng() % 2 + 1,
				                  {Blocks::kLeaves, BlockMetas::Tree::kOak});
			}
		}

		template <typename RNG>
		inline auto GenBirchTree(RNG &rng, int32_t x, int32_t y, int32_t z) -> decltype(rng() - 1, void()) {
			int32_t trunk_height = rng() % 6 + 10;
			int32_t main_crown_size = rng() % 2 + 1;
			for (int32_t i = 0; i < trunk_height + main_crown_size; ++i)
				SetBlock(x, i + y, z, {Blocks::kLog, BlockMetas::Tree::kBirch});
			set_block_cluster(x, y + trunk_height, z, main_crown_size, {Blocks::kLeaves, BlockMetas::Tree::kBirch});

			uint32_t branch_count = rng() % 2 + 4;
			for (uint32_t i = 0; i < branch_count; ++i) {
				int32_t branch_height = trunk_height + main_crown_size - rng() % 4, bry = y + branch_height;

				glm::i32vec3 d = {rng() % 9 - 4, rng() % 3, rng() % 9 - 4};
				int32_t crown_size = (d.x * d.x + d.y * d.y + d.z * d.z >= 9) ? 2 : (rng() % 2 + 1);
				set_block_line(x, bry, z, d.x, d.y, d.z, {Blocks::kLog, BlockMetas::Tree::kBirch});
				set_block_cluster(x + d.x, bry + d.y, z + d.z, crown_size, {Blocks::kLeaves, BlockMetas::Tree::kBirch});
			}
		}

		template <typename RNG>
		inline auto GenAcaciaTree(RNG &rng, int32_t x, int32_t y, int32_t z) -> decltype(rng() - 1, void()) {
			int32_t trunk_height = rng() % 4 + 4;
			for (int32_t i = 0; i < trunk_height; ++i)
				SetBlock(x, i + y, z, {Blocks::kLog, BlockMetas::Tree::kAcacia});

			uint32_t branch_count = rng() % 4 + 2;
			for (uint32_t i = 0; i < branch_count; ++i) {
				int32_t branch_height = trunk_height - 1 + rng() % 2, bry = y + branch_height;

				glm::i32vec3 d = {rng() % 7 - 3, rng() % 3 + 1, rng() % 7 - 3};
				set_block_line(x, bry, z, d.x, d.y, d.z, {Blocks::kLog, BlockMetas::Tree::kAcacia});

				// generate crown
				int32_t crown_size = rng() % 3 + 1;
				set_block_disc(x + d.x, bry + d.y, z + d.z, crown_size, {Blocks::kLeaves, BlockMetas::Tree::kAcacia});
				if (rng() % (4 - crown_size) == 0 && crown_size > 1)
					set_block_disc(x + d.x, bry + d.y + 1, z + d.z, 1, {Blocks::kLeaves, BlockMetas::Tree::kAcacia});
			}
		}
	};
	struct XZInfo {
		DecorationGroup decorations;
		int32_t height_map[kChunkSize * kChunkSize]{}, max_height{INT32_MIN};
		int32_t light_map[kChunkSize * kChunkSize]{}; // the lowest spot with sunlight
		uint16_t meta[kChunkSize * kChunkSize]{};
		Biome biome_map[kChunkSize * kChunkSize]{};
		std::bitset<kChunkSize * kChunkSize> is_ground, is_ocean;
	};
	struct CombinedXZInfo {
		DecorationInfo decoration;
		int32_t light_map[kChunkSize * kChunkSize]{}; // the lowest spot with sunlight
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
	}
	~DefaultTerrain() override = default;
	inline static std::unique_ptr<TerrainBase> Create(uint32_t seed) { return std::make_unique<DefaultTerrain>(seed); }
	void Generate(const std::shared_ptr<Chunk> &chunk_ptr, int32_t light_map[kChunkSize * kChunkSize]) override;
};

#endif
