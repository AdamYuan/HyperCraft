#include <client/DefaultTerrain.hpp>

#include <client/Chunk.hpp>
#include <random>

#include <FastNoise/FastNoise.h>

void DefaultTerrain::Generate(const std::shared_ptr<Chunk> &chunk_ptr, uint32_t *y_peak) {
	/* FastNoise::SmartNode<> noise_enerator = FastNoise::NewFromEncodedNodeTree(
	    "EQACAAAAAAAgQBAAAAAAQBkAEwDD9Sg/"
	    "DQAEAAAAAAAgQAkAAGZmJj8AAAAAPwEEAAAAAAAAAEBAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAM3MTD4AMzMzPwAAAAA/");

	// Generate a 16 x 16 x 16 area of noise
	float noise_output[Chunk::kSize * Chunk::kSize * Chunk::kSize];
	uint32_t noise_index = 0;
	noise_enerator->GenUniformGrid3D(noise_output, chunk_ptr->GetPosition().x * (int)Chunk::kSize,
	                                 chunk_ptr->GetPosition().y * (int)Chunk::kSize,
	                                 chunk_ptr->GetPosition().z * (int)Chunk::kSize, Chunk::kSize, Chunk::kSize,
	                                 Chunk::kSize, 0.005f, (int)GetSeed());

	for (uint32_t z = 0; z < Chunk::kSize; z++) {
	    for (uint32_t y = 0; y < Chunk::kSize; y++) {
	        for (uint32_t x = 0; x < Chunk::kSize; x++) {
	            chunk_ptr->SetBlock(x, y, z, noise_output[noise_index++] < 1 ? Blocks::kSand : Blocks::kAir);
	        }
	    }
	}*/
	std::mt19937 gen(chunk_ptr->GetPosition().x ^ chunk_ptr->GetPosition().y ^ chunk_ptr->GetPosition().z);
	if (chunk_ptr->GetPosition().y < 0) {
		for (uint32_t i = 0; i < 1000; ++i)
			chunk_ptr->SetBlock(gen() % Chunk::kSize, gen() % Chunk::kSize, gen() % Chunk::kSize, gen() % 8);
	} else {
		for (uint32_t i = 0; i < 10; ++i)
			chunk_ptr->SetBlock(gen() % Chunk::kSize, gen() % Chunk::kSize, gen() % Chunk::kSize, gen() % 8);
	}
}
