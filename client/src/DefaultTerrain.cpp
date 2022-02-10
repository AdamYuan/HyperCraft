#include <client/DefaultTerrain.hpp>

#include <client/Chunk.hpp>
#include <random>

#include <FastNoise/FastNoise.h>

void DefaultTerrain::Generate(const std::shared_ptr<Chunk> &chunk_ptr, uint32_t *y_peak) {
#if 1
	FastNoise::SmartNode<> terrain_noise = FastNoise::NewFromEncodedNodeTree(
	    "EQACAAAAAAAgQBAAAAAAQBkAEwDD9Sg/"
	    "DQAEAAAAAAAgQAkAAGZmJj8AAAAAPwEEAAAAAAAAAEBAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAM3MTD4AMzMzPwAAAAA/");
	FastNoise::SmartNode<> cave_noise = FastNoise::NewFromEncodedNodeTree(
	    "EwCamZk+GgABEQACAAAAAADgQBAAAACIQR8AFgABAAAACwADAAAAAgAAAAMAAAAEAAAAAAAAAD8BFAD//wAAAAAAAD8AAAAAPwAAAAA/"
	    "AAAAAD8BFwAAAIC/AACAPz0KF0BSuB5AEwAAAKBABgAAj8J1PACamZk+AAAAAAAA4XoUPw==");

	// Generate a 16 x 16 x 16 area of noise
	float terrain_noise_output[Chunk::kSize * Chunk::kSize * Chunk::kSize],
	    cave_noise_output[Chunk::kSize * Chunk::kSize * Chunk::kSize];
	terrain_noise->GenUniformGrid3D(terrain_noise_output, chunk_ptr->GetPosition().x * (int)Chunk::kSize,
	                                chunk_ptr->GetPosition().y * (int)Chunk::kSize,
	                                chunk_ptr->GetPosition().z * (int)Chunk::kSize, Chunk::kSize, Chunk::kSize,
	                                Chunk::kSize, 0.005f, (int)GetSeed());
	cave_noise->GenUniformGrid3D(cave_noise_output, chunk_ptr->GetPosition().x * (int)Chunk::kSize,
	                             chunk_ptr->GetPosition().y * (int)Chunk::kSize,
	                             chunk_ptr->GetPosition().z * (int)Chunk::kSize, Chunk::kSize, Chunk::kSize,
	                             Chunk::kSize, 0.02f, (int)GetSeed());

	uint32_t noise_index = 0;
	for (uint32_t z = 0; z < Chunk::kSize; ++z) {
		for (uint32_t y = 0; y < Chunk::kSize; ++y) {
			for (uint32_t x = 0; x < Chunk::kSize; ++x, ++noise_index) {
				chunk_ptr->SetBlock(x, y, z,
				                    terrain_noise_output[noise_index] < 0 && cave_noise_output[noise_index] < 0
				                        ? Blocks::kSand
				                        : Blocks::kAir);
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
