#include <cctype>
#include <cstring>
#include <fstream>
#include <iostream>
#include <vector>

#include <stb_image.h>

#include "bytes_to_array.hpp"
#include "write_png.hpp"

int main(int argc, char **argv) {
	--argc;
	++argv;

	if (argc != 1)
		return EXIT_FAILURE;

	// png content
	std::ofstream output{argv[0]};

	// lightmap
	{
		std::vector<stbi_uc> combined_texture(2 * 16 * 16 * 4);
		constexpr const char *kLightmapFilenames[2] = {"lightmap_day.png", "lightmap_night.png"};
		for (uint32_t i = 0; i < std::size(kLightmapFilenames); ++i) {
			const char *filename = kLightmapFilenames[i];
			int x, y, c;
			stbi_uc *img = stbi_load(filename, &x, &y, &c, 4);
			if (img == nullptr) {
				printf("unable to load %s\n", filename);
				return EXIT_FAILURE;
			}
			if (x != y || x != 16) {
				printf("texture %s (%dx%d) is not a 16x16 square\n", filename, x, y);
				stbi_image_free(img);
				return EXIT_FAILURE;
			}
			std::copy(img, img + x * x * 4, combined_texture.data() + 16 * 16 * 4 * i);
		}
		std::vector<unsigned char> png_buffer =
		    WritePngToMemory(16, 16 * std::size(kLightmapFilenames), combined_texture.data());
		printf("PNG size: %f KB\n", png_buffer.size() / 1024.0f);
		output << bytes_to_array("kLightmapTexturePng", png_buffer.data(), png_buffer.size());
	}

	return EXIT_SUCCESS;
}
