#include <client/DefaultTerrain.hpp>

#include <client/Chunk.hpp>
#include <climits>
#include <glm/gtc/type_ptr.hpp>
#include <random>

void DefaultTerrain::Generate(const std::shared_ptr<Chunk> &chunk_ptr, uint32_t *y_peak) {
#if 1
	std::shared_ptr<const XZInfo> xz_info = m_xz_cache.Acquire(
	    chunk_ptr->GetPosition().xz(), [this](const ChunkPos2 &pos, XZInfo *info) { generate_xz_info(pos, info); });

	// base biome blocks
	for (uint32_t y = 0; y < Chunk::kSize; ++y) {
		uint32_t noise_index = 0;
		for (uint32_t z = 0; z < Chunk::kSize; ++z) {
			for (uint32_t x = 0; x < Chunk::kSize; ++x, ++noise_index) {
				int32_t height = xz_info->height_map[noise_index],
				        cur_height = chunk_ptr->GetPosition().y * (int)Chunk::kSize + (int)y;
				Biome biome = xz_info->biome_map[noise_index];
				auto meta = xz_info->meta[noise_index];

				switch (biome) {
				case Biomes::kOcean: {
					if (cur_height <= height) {
						int32_t sand_height = -int32_t(meta % 4u);
						chunk_ptr->SetBlock(x, y, z, (cur_height >= sand_height ? Blocks::kSand : Blocks::kStone));
					} else if (cur_height <= 0) {
						chunk_ptr->SetBlock(x, y, z, Blocks::kWater);
					}
				} break;
				case Biomes::kPlain:
					if (cur_height == height) {
						chunk_ptr->SetBlock(x, y, z, {Blocks::kGrass, BlockMetas::Grass::kPlain});
					} else if (cur_height < height) {
						int32_t dirt_height = height - int32_t(meta % 4u) - 1;
						chunk_ptr->SetBlock(x, y, z, cur_height >= dirt_height ? Blocks::kDirt : Blocks::kStone);
					}
					break;
				case Biomes::kSavanna:
					if (cur_height == height) {
						chunk_ptr->SetBlock(x, y, z, {Blocks::kGrass, BlockMetas::Grass::kSavanna});
					} else if (cur_height < height) {
						int32_t dirt_height = height - int32_t(meta % 6u) - 3;
						chunk_ptr->SetBlock(x, y, z, cur_height >= dirt_height ? Blocks::kDirt : Blocks::kStone);
					}
					break;
				case Biomes::kMountain: {
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
				case Biomes::kSnowMountain: {
					if (cur_height <= height) {
						if ((meta % (std::max(200 - height, 1))) == 0) {
							int32_t ice_height = height - int32_t(meta % 2u);
							chunk_ptr->SetBlock(x, y, z, cur_height >= ice_height ? Blocks::kBlueIce : Blocks::kStone);
						} else if ((meta % (std::max(64 - height, 1))) == 0) {
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
				case Biomes::kForest: {
					if (cur_height == height) {
						chunk_ptr->SetBlock(x, y, z, Block{Blocks::kGrass, BlockMetas::Grass::kPlain});
					} else if (cur_height < height) {
						int32_t dirt_height = height - int32_t(meta % 6u) - 3;
						chunk_ptr->SetBlock(x, y, z, cur_height >= dirt_height ? Blocks::kDirt : Blocks::kStone);
					}
				} break;
				case Biomes::kTropicalForest: {
					if (cur_height == height) {
						chunk_ptr->SetBlock(x, y, z, Block{Blocks::kGrass, BlockMetas::Grass::kTropical});
					} else if (cur_height < height) {
						int32_t dirt_height = height - int32_t(meta % 6u) - 3;
						chunk_ptr->SetBlock(x, y, z, cur_height >= dirt_height ? Blocks::kDirt : Blocks::kStone);
					}
				} break;
				case Biomes::kBorealForest: {
					if (cur_height == height) {
						chunk_ptr->SetBlock(x, y, z, Block{Blocks::kGrass, BlockMetas::Grass::kBoreal});
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
		float cave_noise_output[Chunk::kSize * Chunk::kSize * Chunk::kSize];
		m_cave_noise->GenUniformGrid3D(cave_noise_output, chunk_ptr->GetPosition().x * (int)Chunk::kSize,
		                               chunk_ptr->GetPosition().y * (int)Chunk::kSize,
		                               chunk_ptr->GetPosition().z * (int)Chunk::kSize, Chunk::kSize, Chunk::kSize,
		                               Chunk::kSize, kCaveNoiseFrequency, (int)GetSeed());

		for (uint32_t z = 0, noise_index = 0; z < Chunk::kSize; ++z) {
			for (uint32_t y = 0; y < Chunk::kSize; ++y) {
				for (uint32_t x = 0; x < Chunk::kSize; ++x, ++noise_index) {
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
#else
	// pressure test
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

	m_biome_bias_x_remap = FastNoise::New<FastNoise::Remap>();
	m_biome_bias_x_remap->SetRemap(-1.0, 1.0, -0.5, 0.5);
	m_biome_bias_x_remap->SetSource(m_biome_precipitation_noise);

	m_biome_bias_y_remap = FastNoise::New<FastNoise::Remap>();
	m_biome_bias_y_remap->SetRemap(-1.0, 1.0, -0.5, 0.5);
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
void DefaultTerrain::generate_xz_info(const ChunkPos2 &pos, XZInfo *info) {
	int32_t base_x = (int32_t)pos.x * (int32_t)Chunk::kSize, base_z = (int32_t)pos.y * (int32_t)Chunk::kSize;

	// Generate a 16 x 16 x 16 area of noise
	float biome_temperature_output[Chunk::kSize * Chunk::kSize],
	    biome_precipitation_output[Chunk::kSize * Chunk::kSize],
	    biome_temperature_cell_output[Chunk::kSize * Chunk::kSize],
	    biome_precipitation_cell_output[Chunk::kSize * Chunk::kSize], height_output[Chunk::kSize * Chunk::kSize];
	m_biome_precipitation_cache->GenUniformGrid2D(biome_precipitation_output, base_x, base_z, Chunk::kSize,
	                                              Chunk::kSize, kBiomeNoiseFrequency * kBiomeCellLookupFrequency,
	                                              (int)GetSeed());
	m_biome_temperature_cache->GenUniformGrid2D(biome_temperature_output, base_x, base_z, Chunk::kSize, Chunk::kSize,
	                                            kBiomeNoiseFrequency * kBiomeCellLookupFrequency, (int)GetSeed());
	m_biome_precipitation_cell_cache->GenUniformGrid2D(biome_precipitation_cell_output, base_x, base_z, Chunk::kSize,
	                                                   Chunk::kSize, kBiomeNoiseFrequency, (int)GetSeed());
	m_biome_temperature_cell_cache->GenUniformGrid2D(biome_temperature_cell_output, base_x, base_z, Chunk::kSize,
	                                                 Chunk::kSize, kBiomeNoiseFrequency, (int)GetSeed());
	m_height_noise_cache->GenUniformGrid2D(height_output, base_x, base_z, Chunk::kSize, Chunk::kSize,
	                                       kHeightNoiseFrequency, (int)GetSeed());

	std::vector<float> surface_x_vec, surface_y_vec, surface_z_vec;
	info->max_height = INT32_MIN;
	for (uint32_t index = 0, x, z; index < kChunkSize * kChunkSize; ++index) {
		x = index % kChunkSize;
		z = index / kChunkSize;

		info->height_map[index] =
		    get_height(biome_precipitation_output[index], biome_temperature_output[index], height_output[index]);
		info->max_height = std::max(info->max_height, info->height_map[index]);
		info->biome_map[index] = get_biome(biome_precipitation_cell_output[index], biome_temperature_cell_output[index],
		                                   height_output[index]);

		surface_x_vec.push_back(float(x) * kCaveNoiseFrequency);
		surface_z_vec.push_back(float(z) * kCaveNoiseFrequency);
		surface_y_vec.push_back(float(info->height_map[index]) * kCaveNoiseFrequency);
	}

	float surface_cave_output[kChunkSize * kChunkSize];
	m_cave_noise->GenPositionArray3D(surface_cave_output, kChunkSize * kChunkSize, surface_x_vec.data(),
	                                 surface_y_vec.data(), surface_z_vec.data(), (float)base_x * kCaveNoiseFrequency,
	                                 0.0f, (float)base_z * kCaveNoiseFrequency, (int)GetSeed());

	std::minstd_rand rand_gen((uint32_t)cash(pos.x, pos.y));
	for (uint32_t index = 0; index < kChunkSize * kChunkSize; ++index) {
		uint16_t rand = rand_gen();
		info->meta[index] = rand;
		info->is_ground[index] = surface_cave_output[index] < 0.0f;

		// if not on ground, skip tree generation
		if (!info->is_ground[index])
			continue;
		int32_t x = (int32_t)(index % kChunkSize), z = (int32_t)(index / kChunkSize), y = info->height_map[index];
		// generate trees
		switch (info->biome_map[index]) {
		case Biomes::kForest: {
			if (rand % 100 == 0) {
				if (rand_gen() % 7 == 0)
					info->decorations.GenBirchTree(rand_gen, x, y, z);
				else
					info->decorations.GenOakTree(rand_gen, x, y, z);
			}
		} break;
		case Biomes::kBorealForest: {
			if (rand % 120 == 0)
				info->decorations.GenSpruceTree(rand_gen, x, y, z);
		} break;
		case Biomes::kTropicalForest: {
			if (rand % 80 == 0)
				info->decorations.GenJungleTree(rand_gen, x, y, z);
		} break;
		case Biomes::kPlain: {
			if (rand % 1000 == 0) {
				if (rand_gen() % 5 == 0)
					info->decorations.GenBirchTree(rand_gen, x, y, z);
				else
					info->decorations.GenOakTree(rand_gen, x, y, z);
			}
		} break;
		case Biomes::kSavanna: {
			if (rand % 1200 == 0)
				info->decorations.GenAcaciaTree(rand_gen, x, y, z);
		} break;
		default:
			break;
		}
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
		if (!i.second.GetTransparent() || chunk_ptr->GetBlock(idx) == Blocks::kAir || i.second.GetID() == Blocks::kAir)
			chunk_ptr->SetBlock(idx, i.second);
	}
}
