#include <client/DefaultTerrain.hpp>

#include <client/Chunk.hpp>
#include <climits>
#include <random>

void DefaultTerrain::Generate(const std::shared_ptr<Chunk> &chunk_ptr, uint32_t *y_peak) {
#if 1
	std::shared_ptr<XZInfo> xz_info = m_xz_cache.Acquire(chunk_ptr->GetPosition().xz());
	m_xz_cache.Acquire({chunk_ptr->GetPosition().x + 1, chunk_ptr->GetPosition().z + 1});
	m_xz_cache.Acquire({chunk_ptr->GetPosition().x + 1, chunk_ptr->GetPosition().z});
	m_xz_cache.Acquire({chunk_ptr->GetPosition().x + 1, chunk_ptr->GetPosition().z - 1});
	m_xz_cache.Acquire({chunk_ptr->GetPosition().x, chunk_ptr->GetPosition().z + 1});
	m_xz_cache.Acquire({chunk_ptr->GetPosition().x, chunk_ptr->GetPosition().z - 1});
	m_xz_cache.Acquire({chunk_ptr->GetPosition().x - 1, chunk_ptr->GetPosition().z + 1});
	m_xz_cache.Acquire({chunk_ptr->GetPosition().x - 1, chunk_ptr->GetPosition().z});
	m_xz_cache.Acquire({chunk_ptr->GetPosition().x - 1, chunk_ptr->GetPosition().z - 1});
	for (uint32_t y = 0; y < Chunk::kSize; ++y) {
		uint32_t noise_index = 0;
		for (uint32_t z = 0; z < Chunk::kSize; ++z) {
			for (uint32_t x = 0; x < Chunk::kSize; ++x, ++noise_index) {
				int32_t height = xz_info->height_map[noise_index],
				        cur_height = chunk_ptr->GetPosition().y * (int)Chunk::kSize + (int)y;
				Biome biome = xz_info->biome_map[noise_index];
				auto meta = uint32_t((xz_info->meta[noise_index] + 1.0f) * 1024.0f);

				switch (biome) {
				case Biomes::kOcean: {
					int32_t sand_height = -int32_t(meta % 4u);
					chunk_ptr->SetBlock(x, y, z,
					                    cur_height <= height
					                        ? (cur_height >= sand_height ? Blocks::kSand : Blocks::kStone)
					                        : (cur_height <= 0 ? Blocks::kWater : Blocks::kAir));
				} break;
				case Biomes::kPlain:
					if (cur_height == height) {
						chunk_ptr->SetBlock(x, y, z, Blocks::kGrass);
					} else if (cur_height < height) {
						int32_t dirt_height = height - int32_t(meta % 4u) - 1;
						chunk_ptr->SetBlock(x, y, z, cur_height >= dirt_height ? Blocks::kDirt : Blocks::kStone);
					}
					break;
				case Biomes::kSavanna:
					if (cur_height == height) {
						chunk_ptr->SetBlock(x, y, z, Blocks::kDirt);
					} else if (cur_height < height) {
						int32_t dirt_height = height - int32_t(meta % 6u) - 3;
						chunk_ptr->SetBlock(x, y, z, cur_height >= dirt_height ? Blocks::kDirt : Blocks::kStone);
					}
					break;
				case Biomes::kMountain: {
					if (cur_height <= height) {
						if ((meta % (height + 40)) == 0) {
							int32_t dirt_height = height - int32_t(meta % 3u);
							chunk_ptr->SetBlock(x, y, z, cur_height >= dirt_height ? Blocks::kDirt : Blocks::kStone);
						} else if ((meta % (std::max(60 - height, 1))) == 0) {
							int32_t snow_height = height - int32_t(meta % 2u);
							chunk_ptr->SetBlock(x, y, z, cur_height >= snow_height ? Blocks::kSnow : Blocks::kStone);
						} else
							chunk_ptr->SetBlock(x, y, z, Blocks::kStone);
					}
				} break;
				case Biomes::kSnowMountain: {
					if (cur_height <= height) {
						if ((meta % (std::max(60 - height, 1))) == 0) {
							int32_t ice_height = height - int32_t(meta % 2u);
							chunk_ptr->SetBlock(x, y, z, cur_height >= ice_height ? Blocks::kBlueIce : Blocks::kStone);
						} else if ((meta % (std::max(30 - height, 1))) == 0) {
							int32_t snow_height = height - int32_t(meta % 2u);
							chunk_ptr->SetBlock(x, y, z, cur_height >= snow_height ? Blocks::kSnow : Blocks::kStone);
						} else
							chunk_ptr->SetBlock(x, y, z, Blocks::kStone);
					}
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
				case Biomes::kForest:
				case Biomes::kRainForest:
				case Biomes::kBorealForest: {
					bool tree = (meta % 50u) == 0u;
					if (cur_height == height) {
						chunk_ptr->SetBlock(x, y, z, tree ? Blocks::kLog : Blocks::kGrass);
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

	if (chunk_ptr->GetPosition().y * (int)kChunkSize <= xz_info->max_height) {
		// Generate a 16 x 16 x 16 area of noise
		float cave_noise_output[Chunk::kSize * Chunk::kSize * Chunk::kSize];
		m_cave_noise->GenUniformGrid3D(cave_noise_output, chunk_ptr->GetPosition().x * (int)Chunk::kSize,
		                               chunk_ptr->GetPosition().y * (int)Chunk::kSize,
		                               chunk_ptr->GetPosition().z * (int)Chunk::kSize, Chunk::kSize, Chunk::kSize,
		                               Chunk::kSize, 0.016f, (int)GetSeed());

		for (uint32_t z = 0, noise_index = 0; z < Chunk::kSize; ++z) {
			for (uint32_t y = 0; y < Chunk::kSize; ++y) {
				for (uint32_t x = 0; x < Chunk::kSize; ++x, ++noise_index) {
					if (cave_noise_output[noise_index] >= 0 && chunk_ptr->GetBlock(x, y, z) != Blocks::kWater)
						chunk_ptr->SetBlock(x, y, z, Blocks::kAir);
				}
			}
		}
	}
#else
	std::mt19937 gen(chunk_ptr->GetPosition().x ^ chunk_ptr->GetPosition().y ^ chunk_ptr->GetPosition().z);
	if (chunk_ptr->GetPosition().y <= 0) {
		for (uint32_t i = 0; i < 1000; ++i)
			chunk_ptr->SetBlock(gen() % Chunk::kSize, gen() % Chunk::kSize, gen() % Chunk::kSize, gen() % 8);
	} else {
		for (uint32_t i = 0; i < 10; ++i)
			chunk_ptr->SetBlock(gen() % Chunk::kSize, gen() % Chunk::kSize, gen() % Chunk::kSize, gen() % 8);
	}
#endif
}
void DefaultTerrain::initialize_biome_noise() {
	m_biome_precipitation_noise = FastNoise::New<FastNoise::Perlin>();

	m_biome_temperature_noise = FastNoise::New<FastNoise::SeedOffset>();
	m_biome_temperature_noise->SetSource(m_biome_precipitation_noise);
	m_biome_temperature_noise->SetOffset(100007);

	m_biome_bias_remap_x = FastNoise::New<FastNoise::Remap>();
	m_biome_bias_remap_x->SetRemap(-1.0f, 1.0f, -0.5f, 0.5f);
	m_biome_bias_remap_x->SetSource(m_biome_precipitation_noise);

	m_biome_bias_remap_y = FastNoise::New<FastNoise::Remap>();
	m_biome_bias_remap_y->SetRemap(-1.0f, 1.0f, -0.5f, 0.5f);
	m_biome_bias_remap_y->SetSource(m_biome_temperature_noise);

	m_biome_bias_x = FastNoise::New<FastNoise::FractalFBm>();
	m_biome_bias_x->SetGain(0.5f);
	m_biome_bias_x->SetLacunarity(3.0f);
	m_biome_bias_x->SetOctaveCount(5);
	m_biome_bias_x->SetWeightedStrength(0);
	m_biome_bias_x->SetSource(m_biome_bias_remap_x);

	m_biome_bias_y = FastNoise::New<FastNoise::FractalFBm>();
	m_biome_bias_y->SetGain(0.5f);
	m_biome_bias_y->SetLacunarity(3.0f);
	m_biome_bias_y->SetOctaveCount(5);
	m_biome_bias_y->SetWeightedStrength(0);
	m_biome_bias_y->SetSource(m_biome_bias_remap_y);

	/* m_biome_precipitation_cellular_lookup = FastNoise::New<FastNoise::CellularLookup>();
	m_biome_precipitation_cellular_lookup->SetLookupFrequency(0.1f);
	m_biome_precipitation_cellular_lookup->SetDistanceFunction(FastNoise::DistanceFunction::EuclideanSquared);
	m_biome_precipitation_cellular_lookup->SetJitterModifier(0.75f);
	m_biome_precipitation_cellular_lookup->SetLookup(m_biome_precipitation_noise);

	m_biome_temperature_cellular_lookup = FastNoise::New<FastNoise::CellularLookup>();
	m_biome_temperature_cellular_lookup->SetLookupFrequency(0.1f);
	m_biome_temperature_cellular_lookup->SetDistanceFunction(FastNoise::DistanceFunction::EuclideanSquared);
	m_biome_temperature_cellular_lookup->SetJitterModifier(0.75f);
	m_biome_temperature_cellular_lookup->SetLookup(m_biome_temperature_noise); */

	m_biome_precipitation = FastNoise::New<FastNoise::DomainOffset>();
	m_biome_precipitation->SetOffset<FastNoise::Dim::X>(m_biome_bias_x);
	m_biome_precipitation->SetOffset<FastNoise::Dim::Y>(m_biome_bias_y);
	m_biome_precipitation->SetSource(m_biome_precipitation_noise);

	m_biome_temperature = FastNoise::New<FastNoise::DomainOffset>();
	m_biome_temperature->SetOffset<FastNoise::Dim::X>(m_biome_bias_x);
	m_biome_temperature->SetOffset<FastNoise::Dim::Y>(m_biome_bias_y);
	m_biome_temperature->SetSource(m_biome_temperature_noise);

	m_biome_precipitation_cache = FastNoise::New<FastNoise::GeneratorCache>();
	m_biome_precipitation_cache->SetSource(m_biome_precipitation);

	m_biome_temperature_cache = FastNoise::New<FastNoise::GeneratorCache>();
	m_biome_temperature_cache->SetSource(m_biome_temperature);
}

void DefaultTerrain::initialize_height_noise() {
	m_height_noise = FastNoise::New<FastNoise::FractalFBm>();
	m_height_noise->SetGain(0.5f);
	m_height_noise->SetLacunarity(2.5f);
	m_height_noise->SetOctaveCount(5);
	m_height_noise->SetWeightedStrength(0);
	m_height_noise->SetSource(m_biome_precipitation_noise);

	m_height_cache = FastNoise::New<FastNoise::GeneratorCache>();
	m_height_cache->SetSource(m_height_noise);
}

void DefaultTerrain::initialize_cave_noise() {
	m_cave_noise = FastNoise::NewFromEncodedNodeTree(
	    "EwCamZk+GgABEQACAAAAAADgQBAAAACIQR8AFgABAAAACwADAAAAAgAAAAMAAAAEAAAAAAAAAD8BFAD//wAAAAAAAD8AAAAAPwAAAAA/"
	    "AAAAAD8BFwAAAIC/AACAPz0KF0BSuB5AEwAAAKBABgAAj8J1PACamZk+AAAAAAAA4XoUPw==");
}

void DefaultTerrain::initialize_meta_noise() {
	m_meta_noise = FastNoise::New<FastNoise::White>();

	m_meta_cache = FastNoise::New<FastNoise::GeneratorCache>();
	m_meta_cache->SetSource(m_meta_noise);
}

void DefaultTerrain::generate_xz_info(const ChunkPos2 &pos, XZInfo *info) {
	// Generate a 16 x 16 x 16 area of noise
	float biome_temperature_output[Chunk::kSize * Chunk::kSize],
	    biome_precipitation_output[Chunk::kSize * Chunk::kSize], height_output[Chunk::kSize * Chunk::kSize];
	m_biome_precipitation_cache->GenUniformGrid2D(biome_precipitation_output, (int)pos.x * (int)Chunk::kSize,
	                                              (int)pos.y * (int)Chunk::kSize, Chunk::kSize, Chunk::kSize,
	                                              kBiomeNoiseFrequency, (int)GetSeed());

	m_biome_temperature_cache->GenUniformGrid2D(biome_temperature_output, (int)pos.x * (int)Chunk::kSize,
	                                            (int)pos.y * (int)Chunk::kSize, Chunk::kSize, Chunk::kSize,
	                                            kBiomeNoiseFrequency, (int)GetSeed());

	m_height_cache->GenUniformGrid2D(height_output, (int)pos.x * (int)Chunk::kSize, (int)pos.y * (int)Chunk::kSize,
	                                 Chunk::kSize, Chunk::kSize, kHeightNoiseFrequency, (int)GetSeed());

	m_meta_cache->GenUniformGrid2D(info->meta, (int)pos.x * (int)Chunk::kSize, (int)pos.y * (int)Chunk::kSize,
	                               Chunk::kSize, Chunk::kSize, 1.0f, (int)GetSeed());

	info->max_height = INT32_MIN;
	for (uint32_t noise_index = 0; noise_index < Chunk::kSize * Chunk::kSize; ++noise_index) {
		info->height_map[noise_index] = get_height(biome_precipitation_output[noise_index],
		                                           biome_temperature_output[noise_index], height_output[noise_index]);
		info->max_height = std::max(info->max_height, info->height_map[noise_index]);
		info->biome_map[noise_index] = get_biome(biome_precipitation_output[noise_index],
		                                         biome_temperature_output[noise_index], height_output[noise_index]);
	}
}
