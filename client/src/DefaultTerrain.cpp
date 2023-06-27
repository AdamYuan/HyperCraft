#include <client/DefaultTerrain.hpp>

#include <client/Chunk.hpp>
#include <climits>
#include <glm/gtc/type_ptr.hpp>
#include <random>

namespace hc::client {

void DefaultTerrain::Generate(const std::shared_ptr<Chunk> &chunk_ptr, int32_t light_map[kChunkSize * kChunkSize]) {
#if 1
	std::shared_ptr<const XZInfo> xz_info = m_xz_cache.Acquire(
	    chunk_ptr->GetPosition().xz(), [this](const ChunkPos2 &pos, XZInfo *info) { generate_xz_info(pos, info); });

	// base biome blocks
	for (uint32_t y = 0; y < kChunkSize; ++y) {
		uint32_t noise_index = 0;
		for (uint32_t z = 0; z < kChunkSize; ++z) {
			for (uint32_t x = 0; x < kChunkSize; ++x, ++noise_index) {
				int32_t height = xz_info->height_map[noise_index],
				        cur_height = chunk_ptr->GetPosition().y * (int)kChunkSize + (int)y;
				Biome biome = xz_info->biome_map[noise_index];
				auto meta = xz_info->meta[noise_index];

				if (xz_info->is_ocean[noise_index]) {
					Block surface = Blocks::kSand;
					if (biome == Biomes::kBorealForest || biome == Biomes::kTropicalForest)
						surface = Blocks::kGravel;
					else if (biome == Biomes::kTundra)
						surface = Blocks::kCobblestone;
					else if (biome == Biomes::kGlacier)
						surface = Blocks::kSnow;
					if (cur_height <= height) {
						int32_t sand_height = -int32_t(meta % 4u);
						chunk_ptr->SetBlock(x, y, z, (cur_height >= sand_height ? surface : Blocks::kStone));
					} else if (cur_height <= 0) {
						chunk_ptr->SetBlock(x, y, z, Blocks::kWater);
					}
					continue;
				}

				switch (biome) {
				case Biomes::kPlain:
					if (cur_height == height) {
						chunk_ptr->SetBlock(x, y, z, {Blocks::kGrassBlock, BlockVariants::Grass::kPlain, 0});
					} else if (cur_height < height) {
						int32_t dirt_height = height - int32_t(meta % 4u) - 1;
						chunk_ptr->SetBlock(x, y, z, cur_height >= dirt_height ? Blocks::kDirt : Blocks::kStone);
					}
					break;
				case Biomes::kSavanna:
					if (cur_height == height) {
						chunk_ptr->SetBlock(x, y, z, {Blocks::kGrassBlock, BlockVariants::Grass::kSavanna, 0});
					} else if (cur_height < height) {
						int32_t dirt_height = height - int32_t(meta % 6u) - 3;
						chunk_ptr->SetBlock(x, y, z, cur_height >= dirt_height ? Blocks::kDirt : Blocks::kStone);
					}
					break;
					// TODO: Better tundra generation
				case Biomes::kTundra: {
					if (cur_height <= height) {
						if ((meta % (height + 64)) == 0) {
							int32_t dirt_height = height - int32_t(meta % 3u);
							chunk_ptr->SetBlock(x, y, z, cur_height >= dirt_height ? Blocks::kDirt : Blocks::kStone);
						} else if ((meta % (std::max(128 - height, 1))) == 0) {
							int32_t snow_height = height - int32_t(meta % 2u);
							chunk_ptr->SetBlock(x, y, z, cur_height >= snow_height ? Blocks::kSnow : Blocks::kStone);
						} else
							chunk_ptr->SetBlock(x, y, z, Blocks::kStone);
					}
				} break;
				case Biomes::kGlacier: {
					int32_t ice_height = height - int32_t(meta % 8u);
					bool snow_cover = xz_info->meta[noise_index] % 16;
					if (cur_height <= height)
						chunk_ptr->SetBlock(
						    x, y, z,
						    cur_height >= ice_height
						        ? (snow_cover && cur_height == height ? Blocks::kSnow : Blocks::kBlueIce)
						        : Blocks::kStone);
				} break;
				case Biomes::kDesert: {
					if (cur_height <= height) {
						int32_t sand_height = height - int32_t(meta % 4u);
						if (sand_height <= cur_height) {
							chunk_ptr->SetBlock(x, y, z, Blocks::kSand);
						} else {
							int32_t sandstone_height = sand_height - int32_t(meta % 6u);
							chunk_ptr->SetBlock(x, y, z,
							                    cur_height >= sandstone_height ? Blocks::kSandstone : Blocks::kStone);
						}
					}
				} break;
				case Biomes::kForest: {
					if (cur_height == height) {
						chunk_ptr->SetBlock(x, y, z, Block{Blocks::kGrassBlock, BlockVariants::Grass::kPlain, 0});
					} else if (cur_height < height) {
						int32_t dirt_height = height - int32_t(meta % 6u) - 3;
						chunk_ptr->SetBlock(x, y, z, cur_height >= dirt_height ? Blocks::kDirt : Blocks::kStone);
					}
				} break;
				case Biomes::kTropicalForest: {
					if (cur_height == height) {
						chunk_ptr->SetBlock(x, y, z, Block{Blocks::kGrassBlock, BlockVariants::Grass::kTropical, 0});
					} else if (cur_height < height) {
						int32_t dirt_height = height - int32_t(meta % 6u) - 3;
						chunk_ptr->SetBlock(x, y, z, cur_height >= dirt_height ? Blocks::kDirt : Blocks::kStone);
					}
				} break;
				case Biomes::kBorealForest: {
					if (cur_height == height) {
						chunk_ptr->SetBlock(x, y, z, Block{Blocks::kGrassBlock, BlockVariants::Grass::kBoreal, 0});
					} else if (cur_height < height) {
						int32_t dirt_height = height - int32_t(meta % 6u) - 3;
						chunk_ptr->SetBlock(x, y, z, cur_height >= dirt_height ? Blocks::kDirt : Blocks::kStone);
					}
				} break;
				default:
					break;
				}
			}
		}
	}
	// cave
	if (chunk_ptr->GetPosition().y * (int)kChunkSize <= xz_info->max_height) {
		// Generate a 16 x 16 x 16 area of noise
		thread_local static float cave_noise_output[kChunkSize * kChunkSize * kChunkSize];
		m_cave_noise->GenUniformGrid3D(cave_noise_output, chunk_ptr->GetPosition().x * (int)kChunkSize,
		                               chunk_ptr->GetPosition().y * (int)kChunkSize,
		                               chunk_ptr->GetPosition().z * (int)kChunkSize, kChunkSize, kChunkSize, kChunkSize,
		                               kCaveNoiseFrequency, (int)GetSeed());

		for (uint32_t z = 0, noise_index = 0; z < kChunkSize; ++z) {
			for (uint32_t y = 0; y < kChunkSize; ++y) {
				for (uint32_t x = 0; x < kChunkSize; ++x, ++noise_index) {
					if (cave_noise_output[noise_index] >= 0.0f && chunk_ptr->GetBlock(x, y, z) != Blocks::kWater)
						chunk_ptr->SetBlock(x, y, z, Blocks::kAir);
				}
			}
		}
	}
	// decorations
	std::shared_ptr<const CombinedXZInfo> combined_xz_info = m_combined_xz_cache.Acquire(
	    chunk_ptr->GetPosition().xz(), [&xz_info, this](const ChunkPos2 &pos, CombinedXZInfo *combined_info) {
		    generate_combined_xz_info(pos, xz_info, combined_info);
	    });
	combined_xz_info->decoration.PopToChunk(chunk_ptr);
	// pop light info
	std::copy(std::begin(combined_xz_info->light_map), std::end(combined_xz_info->light_map), light_map);
#else
	// pressure test
	std::mt19937 gen(chunk_ptr->GetPosition().x ^ chunk_ptr->GetPosition().y ^ chunk_ptr->GetPosition().z);
	if (chunk_ptr->GetPosition().y <= 0) {
		for (uint32_t i = 0; i < 1000; ++i)
			chunk_ptr->SetBlock(gen() % kChunkSize, gen() % kChunkSize, gen() % kChunkSize, gen() % 8);
	} else {
		for (uint32_t i = 0; i < 10; ++i)
			chunk_ptr->SetBlock(gen() % kChunkSize, gen() % kChunkSize, gen() % kChunkSize, gen() % 8);
	}
#endif
}
void DefaultTerrain::initialize_biome_noise() {
	m_biome_precipitation_noise = FastNoise::New<FastNoise::Perlin>();

	m_biome_temperature_noise = FastNoise::New<FastNoise::SeedOffset>();
	m_biome_temperature_noise->SetSource(m_biome_precipitation_noise);
	m_biome_temperature_noise->SetOffset(100007);

	m_biome_bias_x_remap = FastNoise::New<FastNoise::Remap>();
	m_biome_bias_x_remap->SetRemap(-1.0, 1.0, -0.4, 0.4);
	m_biome_bias_x_remap->SetSource(m_biome_precipitation_noise);

	m_biome_bias_y_remap = FastNoise::New<FastNoise::Remap>();
	m_biome_bias_y_remap->SetRemap(-1.0, 1.0, -0.4, 0.4);
	m_biome_bias_y_remap->SetSource(m_biome_temperature_noise);

	m_biome_bias_x = FastNoise::New<FastNoise::FractalFBm>();
	m_biome_bias_x->SetGain(0.5f);
	m_biome_bias_x->SetLacunarity(3.0f);
	m_biome_bias_x->SetOctaveCount(5);
	m_biome_bias_x->SetWeightedStrength(0);
	m_biome_bias_x->SetSource(m_biome_bias_x_remap);

	m_biome_bias_y = FastNoise::New<FastNoise::FractalFBm>();
	m_biome_bias_y->SetGain(0.5f);
	m_biome_bias_y->SetLacunarity(3.0f);
	m_biome_bias_y->SetOctaveCount(5);
	m_biome_bias_y->SetWeightedStrength(0);
	m_biome_bias_y->SetSource(m_biome_bias_y_remap);

	m_biome_precipitation_offset = FastNoise::New<FastNoise::DomainOffset>();
	m_biome_precipitation_offset->SetOffset<FastNoise::Dim::X>(m_biome_bias_x);
	m_biome_precipitation_offset->SetOffset<FastNoise::Dim::Y>(m_biome_bias_y);
	m_biome_precipitation_offset->SetSource(m_biome_precipitation_noise);

	m_biome_temperature_offset = FastNoise::New<FastNoise::DomainOffset>();
	m_biome_temperature_offset->SetOffset<FastNoise::Dim::X>(m_biome_bias_x);
	m_biome_temperature_offset->SetOffset<FastNoise::Dim::Y>(m_biome_bias_y);
	m_biome_temperature_offset->SetSource(m_biome_temperature_noise);

	m_biome_precipitation_cell_lookup = FastNoise::New<FastNoise::CellularLookup>();
	m_biome_precipitation_cell_lookup->SetLookupFrequency(kBiomeCellLookupFrequency);
	m_biome_precipitation_cell_lookup->SetDistanceFunction(FastNoise::DistanceFunction::EuclideanSquared);
	m_biome_precipitation_cell_lookup->SetJitterModifier(0.75f);
	m_biome_precipitation_cell_lookup->SetLookup(m_biome_precipitation_offset);

	m_biome_temperature_cell_lookup = FastNoise::New<FastNoise::CellularLookup>();
	m_biome_temperature_cell_lookup->SetLookupFrequency(kBiomeCellLookupFrequency);
	m_biome_temperature_cell_lookup->SetDistanceFunction(FastNoise::DistanceFunction::EuclideanSquared);
	m_biome_temperature_cell_lookup->SetJitterModifier(0.75f);
	m_biome_temperature_cell_lookup->SetLookup(m_biome_temperature_offset);

	m_biome_precipitation_cell_offset = FastNoise::New<FastNoise::DomainOffset>();
	m_biome_precipitation_cell_offset->SetOffset<FastNoise::Dim::X>(m_biome_bias_x);
	m_biome_precipitation_cell_offset->SetOffset<FastNoise::Dim::Y>(m_biome_bias_y);
	m_biome_precipitation_cell_offset->SetSource(m_biome_precipitation_cell_lookup);

	m_biome_temperature_cell_offset = FastNoise::New<FastNoise::DomainOffset>();
	m_biome_temperature_cell_offset->SetOffset<FastNoise::Dim::X>(m_biome_bias_x);
	m_biome_temperature_cell_offset->SetOffset<FastNoise::Dim::Y>(m_biome_bias_y);
	m_biome_temperature_cell_offset->SetSource(m_biome_temperature_cell_lookup);

	m_biome_precipitation_cell_cache = FastNoise::New<FastNoise::GeneratorCache>();
	m_biome_precipitation_cell_cache->SetSource(m_biome_precipitation_cell_offset);

	m_biome_temperature_cell_cache = FastNoise::New<FastNoise::GeneratorCache>();
	m_biome_temperature_cell_cache->SetSource(m_biome_temperature_cell_offset);

	m_biome_precipitation_cache = FastNoise::New<FastNoise::GeneratorCache>();
	m_biome_precipitation_cache->SetSource(m_biome_precipitation_offset);

	m_biome_temperature_cache = FastNoise::New<FastNoise::GeneratorCache>();
	m_biome_temperature_cache->SetSource(m_biome_temperature_offset);

	// River Noise
	FastNoise::SmartNode<FastNoise::SeedOffset> river_mask_offset = FastNoise::New<FastNoise::SeedOffset>();
	river_mask_offset->SetSource(m_biome_precipitation_noise);
	river_mask_offset->SetOffset(12313);

	m_river_mask_remap = FastNoise::New<FastNoise::Remap>();
	m_river_mask_remap->SetSource(river_mask_offset);
	m_river_mask_remap->SetRemap(0.0f, 0.2f, 0.0f, 1.0f);

	m_river_mask_min = FastNoise::New<FastNoise::Min>();
	m_river_mask_min->SetLHS(m_river_mask_remap);
	m_river_mask_min->SetRHS(1.0f);

	m_river_cell = FastNoise::New<FastNoise::CellularDistance>();
	m_river_cell->SetDistanceFunction(FastNoise::DistanceFunction::EuclideanSquared);
	m_river_cell->SetJitterModifier(0.75f);
	m_river_cell->SetDistanceIndex0(0);
	m_river_cell->SetDistanceIndex1(1);
	m_river_cell->SetReturnType(FastNoise::CellularDistance::ReturnType::Index0Div1);

	m_river_multiply = FastNoise::New<FastNoise::Multiply>();
	m_river_multiply->SetLHS(m_river_mask_min);
	m_river_multiply->SetRHS(m_river_cell);

	m_river_max = FastNoise::New<FastNoise::Max>();
	m_river_max->SetLHS(m_river_multiply);
	m_river_max->SetRHS(0.0f);

	m_river_bias_x = FastNoise::New<FastNoise::FractalFBm>();
	m_river_bias_x->SetGain(0.5f);
	m_river_bias_x->SetLacunarity(3.0f);
	m_river_bias_x->SetOctaveCount(3);
	m_river_bias_x->SetWeightedStrength(0);
	m_river_bias_x->SetSource(m_biome_bias_x_remap);

	m_river_bias_y = FastNoise::New<FastNoise::FractalFBm>();
	m_river_bias_y->SetGain(0.5f);
	m_river_bias_y->SetLacunarity(3.0f);
	m_river_bias_y->SetOctaveCount(3);
	m_river_bias_y->SetWeightedStrength(0);
	m_river_bias_y->SetSource(m_biome_bias_y_remap);

	m_river_offset = FastNoise::New<FastNoise::DomainOffset>();
	m_river_offset->SetOffset<FastNoise::Dim::X>(m_river_bias_x);
	m_river_offset->SetOffset<FastNoise::Dim::Y>(m_river_bias_y);
	m_river_offset->SetSource(m_river_max);
}

void DefaultTerrain::initialize_height_noise() {
	m_height_noise = FastNoise::New<FastNoise::FractalFBm>();
	m_height_noise->SetGain(0.5f);
	m_height_noise->SetLacunarity(2.0f);
	m_height_noise->SetOctaveCount(6);
	m_height_noise->SetWeightedStrength(0);
	m_height_noise->SetSource(m_biome_precipitation_noise);

	m_height_noise_cache = FastNoise::New<FastNoise::GeneratorCache>();
	m_height_noise_cache->SetSource(m_height_noise);
}

void DefaultTerrain::initialize_cave_noise() {
	m_cave_noise = FastNoise::NewFromEncodedNodeTree(
	    "EwCamZk+GgABEQACAAAAAADgQBAAAACIQR8AFgABAAAACwADAAAAAgAAAAMAAAAEAAAAAAAAAD8BFAD//wAAAAAAAD8AAAAAPwAAAAA/"
	    "AAAAAD8BFwAAAIC/AACAPz0KF0BSuB5AEwAAAKBABgAAj8J1PACamZk+AAAAAAAA4XoUPw==");
}

inline int32_t cash(int32_t x, int32_t y) {
	int32_t h = x * 374761393 + y * 668265263; // all constants are prime
	h = (h ^ (h >> 13)) * 1274126177;
	return h ^ (h >> 16);
}
inline static constexpr int32_t ceil32(float x) { return x >= 0 ? (int32_t)x + 1 : (int32_t)x; }
inline static constexpr int32_t floor32(float x) { return x >= 0 ? (int32_t)x : (int32_t)x - 1; }
void DefaultTerrain::generate_xz_info(const ChunkPos2 &pos, XZInfo *info) {
	int32_t base_x = (int32_t)pos.x * (int32_t)kChunkSize, base_z = (int32_t)pos.y * (int32_t)kChunkSize;

	// Generate a 16 x 16 x 16 area of noise
	thread_local static float biome_temperature_output[kChunkSize * kChunkSize],
	    biome_precipitation_output[kChunkSize * kChunkSize], biome_temperature_cell_output[kChunkSize * kChunkSize],
	    biome_precipitation_cell_output[kChunkSize * kChunkSize], height_output[kChunkSize * kChunkSize],
	    river_output[kChunkSize * kChunkSize];
	m_biome_precipitation_cache->GenUniformGrid2D(biome_precipitation_output, base_x, base_z, kChunkSize, kChunkSize,
	                                              kBiomeNoiseFrequency * kBiomeCellLookupFrequency, (int)GetSeed());
	m_biome_temperature_cache->GenUniformGrid2D(biome_temperature_output, base_x, base_z, kChunkSize, kChunkSize,
	                                            kBiomeNoiseFrequency * kBiomeCellLookupFrequency, (int)GetSeed());
	m_biome_precipitation_cell_cache->GenUniformGrid2D(biome_precipitation_cell_output, base_x, base_z, kChunkSize,
	                                                   kChunkSize, kBiomeNoiseFrequency, (int)GetSeed());
	m_biome_temperature_cell_cache->GenUniformGrid2D(biome_temperature_cell_output, base_x, base_z, kChunkSize,
	                                                 kChunkSize, kBiomeNoiseFrequency, (int)GetSeed());
	m_height_noise_cache->GenUniformGrid2D(height_output, base_x, base_z, kChunkSize, kChunkSize, kHeightNoiseFrequency,
	                                       (int)GetSeed());
	m_river_offset->GenUniformGrid2D(river_output, base_x, base_z, kChunkSize, kChunkSize, kBiomeNoiseFrequency,
	                                 (int)GetSeed());

	thread_local static float surface_query_x[kChunkSize * kChunkSize], surface_query_y[kChunkSize * kChunkSize],
	    surface_query_z[kChunkSize * kChunkSize];
	info->max_height = INT32_MIN;
	for (uint32_t index = 0, x, z; index < kChunkSize * kChunkSize; ++index) {
		x = index % kChunkSize;
		z = index / kChunkSize;

		float origin_height = height_output[index];
		float river_depth = river_get_depth(origin_height);
		float final_height = river_modify_range_height(
		    river_output[index], river_depth,
		    biome_modify_height(biome_precipitation_output[index], biome_temperature_output[index], origin_height),
		    biome_precipitation_output[index]);
		info->height_map[index] = ceil32(final_height);

		info->max_height = std::max(info->max_height, info->height_map[index]);
		info->biome_map[index] =
		    get_biome(biome_precipitation_cell_output[index], biome_temperature_cell_output[index]);
		info->is_ocean[index] = info->height_map[index] <= 0;

		info->light_map[index] = info->height_map[index];
		surface_query_x[index] = float(x) * kCaveNoiseFrequency;
		surface_query_z[index] = float(z) * kCaveNoiseFrequency;
		surface_query_y[index] = float(info->light_map[index]) * kCaveNoiseFrequency;
	}

	thread_local static float surface_cave_output[kChunkSize * kChunkSize];
	// spdlog::info("{} {} {}", surface_x_vec.size(), surface_z_vec.size(), surface_y_vec.size());
	m_cave_noise->GenPositionArray3D(surface_cave_output, kChunkSize * kChunkSize, surface_query_x, surface_query_y,
	                                 surface_query_z, (float)base_x * kCaveNoiseFrequency, 0.0f,
	                                 (float)base_z * kCaveNoiseFrequency, (int)GetSeed());

	// Generate decorations
	std::minstd_rand rand_gen((uint32_t)cash(pos.x, pos.y));
	for (uint32_t index = 0; index < kChunkSize * kChunkSize; ++index) {
		uint16_t rand = rand_gen();
		info->meta[index] = rand;

		if (info->is_ocean[index] && info->height_map[index] < 0)
			info->is_ground[index] = false;
		else
			info->is_ground[index] = surface_cave_output[index] < 0.0f;

		int32_t x = (int32_t)(index % kChunkSize), z = (int32_t)(index / kChunkSize), y = info->height_map[index];

		if (info->is_ocean[index]) {
			// generate floating ice
			if (info->biome_map[index] == Biomes::kGlacier) {
				int32_t ice_height = rand % (std::abs(info->height_map[index]) + 1) <= 1 ? -int32_t(rand % 3u) : 0;
				for (int32_t i = ice_height + 1; i <= 0; ++i)
					info->decorations.SetBlock(x, i, z, Blocks::kIce);
			}
			// skip other decorations if ocean
			continue;
		}

		// if not on ground, skip tree generation
		if (!info->is_ground[index])
			continue;
		// generate trees
		switch (info->biome_map[index]) {
		case Biomes::kForest: {
			if (rand % 100 == 0) {
				if (rand_gen() % 7 == 0)
					info->decorations.GenBirchTree(rand_gen, x, y, z);
				else
					info->decorations.GenOakTree(rand_gen, x, y, z);
			} else if (rand % 273 == 0)
				info->decorations.SetBlock(x, y + 1, z, {Blocks::kGrass, BlockVariants::Grass::kPlain, 0});
			else if (rand % 647 == 0)
				info->decorations.SetBlock(x, y + 1, z,
				                           (rand_gen() & 1u) ? Blocks::kRedMushroom : Blocks::kBrownMushroom);
		} break;
		case Biomes::kBorealForest: {
			if (rand % 120 == 0)
				info->decorations.GenSpruceTree(rand_gen, x, y, z);
			else if (rand % 87 == 0)
				info->decorations.SetBlock(x, y + 1, z, {Blocks::kGrass, BlockVariants::Grass::kBoreal, 0});
			else if (rand % 437 == 0)
				info->decorations.SetBlock(x, y + 1, z, Blocks::kBrownMushroom);
		} break;
		case Biomes::kTropicalForest: {
			if (rand % 57 == 0)
				info->decorations.GenJungleTree(rand_gen, x, y, z);
			else if (rand % 127 == 0)
				info->decorations.SetBlock(x, y + 1, z, {Blocks::kGrass, BlockVariants::Grass::kTropical, 0});
			else if (rand % 117 == 0)
				info->decorations.SetBlock(x, y + 1, z,
				                           (rand_gen() & 1u) ? Blocks::kRedMushroom : Blocks::kBrownMushroom);
		} break;
		case Biomes::kPlain: {
			if (rand % 1000 == 0) {
				if (rand_gen() % 5 == 0)
					info->decorations.GenBirchTree(rand_gen, x, y, z);
				else
					info->decorations.GenOakTree(rand_gen, x, y, z);
			} else if (rand % 227 == 0) {
				info->decorations.SetBlock(x, y + 1, z, {Blocks::kGrass, BlockVariants::Grass::kPlain, 0});
			}
		} break;
		case Biomes::kDesert: {
			if (rand % 473 == 0)
				info->decorations.SetBlock(x, y + 1, z, Blocks::kDeadBush);
			else if (y >= 4 && rand % 837 == 0)
				info->decorations.GenCactus(rand_gen, x, y + 1, z);
		} break;
		case Biomes::kSavanna: {
			if (rand % 1200 == 0)
				info->decorations.GenAcaciaTree(rand_gen, x, y, z);
			else if (rand % 257 == 0)
				info->decorations.SetBlock(x, y + 1, z, {Blocks::kGrass, BlockVariants::Grass::kSavanna, 0});
			else if (rand % 537 == 0)
				info->decorations.SetBlock(x, y + 1, z, Blocks::kDeadBush);
		} break;
		default:
			break;
		}
	}

	// Generate light map
	constexpr uint32_t kTestDepth = 16, kMaxTries = 100;
	thread_local static float deep_cave_output[kTestDepth];
	for (uint32_t index = 0; index < kChunkSize * kChunkSize; ++index) {
		if (!Block{Blocks::kWater}.GetVerticalLightPass()) {
			if (info->light_map[index] < 0) {
				info->light_map[index] = 0;
				continue;
			}
		}
		if (!info->is_ground[index]) {
			int32_t x = (int32_t)(index % kChunkSize) + base_x, z = (int32_t)(index / kChunkSize) + base_z,
			        &y = info->light_map[index];
			--y;

			uint32_t tries = kMaxTries;
			while (tries--) {
				y -= (int32_t)kTestDepth;
				m_cave_noise->GenUniformGrid3D(deep_cave_output, x, y + 1, z, 1, kTestDepth, 1, kCaveNoiseFrequency,
				                               (int)GetSeed());
				for (int32_t i = kTestDepth; i > 0; --i) {
					if (deep_cave_output[i - 1] < 0.0f) {
						// find ground
						y += i;
						goto light_map_done;
					}
				}
			}
		}
	light_map_done:;
	}
}

void DefaultTerrain::generate_combined_xz_info(const ChunkPos2 &pos,
                                               const std::shared_ptr<const XZInfo> &center_xz_info,
                                               DefaultTerrain::CombinedXZInfo *combined_info) {
	combined_info->decoration = center_xz_info->decorations.GetInfo(cmp_xz_to_neighbour_index(0, 0));
	auto xz_generator = [this](const ChunkPos2 &pos, XZInfo *info) { generate_xz_info(pos, info); };
	for (uint32_t i = 0; i < 8; ++i) {
		ChunkPos2 dp;
		neighbour_index_to_cmp_xz(i, glm::value_ptr(dp));
		combined_info->decoration.Merge(
		    m_xz_cache.Acquire(pos + dp, xz_generator)->decorations.GetInfo(opposite_xz_neighbour_index(i)));
	}
	std::copy(center_xz_info->light_map, center_xz_info->light_map + kChunkSize * kChunkSize, combined_info->light_map);
	combined_info->decoration.PopToLightMap(combined_info->light_map);
}

void DefaultTerrain::DecorationInfo::PopToChunk(const std::shared_ptr<Chunk> &chunk_ptr) const {
	int32_t base_y = (int32_t)chunk_ptr->GetPosition().y * (int32_t)kChunkSize;
	if (m_y_max < base_y || m_y_min >= base_y + (int32_t)kChunkSize)
		return;
	for (const auto &i : m_blocks) {
		int32_t y = i.first.y - base_y;
		if (y < 0 || y >= kChunkSize)
			continue;
		uint32_t idx = Chunk::XYZ2Index(i.first.x, y, i.first.z);
		// TODO: better override condition
		if (!i.second.GetIndirectLightPass() || chunk_ptr->GetBlock(idx) == Blocks::kAir ||
		    chunk_ptr->GetBlock(idx) == Blocks::kWater || i.second.GetID() == Blocks::kAir ||
		    i.second.GetID() == Blocks::kWater)
			chunk_ptr->SetBlock(idx, i.second);
	}
}

} // namespace hc::client