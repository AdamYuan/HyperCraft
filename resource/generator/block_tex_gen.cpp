#define MAGIC_ENUM_RANGE_MIN 0
#define MAGIC_ENUM_RANGE_MAX 256
#include <magic_enum.hpp>

#include <cctype>
#include <fstream>
#include <iostream>
#include <set>
#include <vector>

#include <png.h>
#include <stb_image.h>

#include "bytes_to_array.hpp"
#include "write_png.hpp"

enum ID {
#include <block_texture_enum.inl>
};

std::set<uint32_t> preserved_opaque_texture = {ID::kApple};

std::string make_texture_filename(std::string_view x) {
	std::string ret;
	bool first_upper = true;
	for (auto i = x.begin() + 1; i != x.end(); ++i) {
		if (isupper(*i)) {
			if (first_upper)
				first_upper = false;
			else
				ret.push_back('_');
			ret.push_back((char)tolower(*i));
		} else
			ret.push_back(*i);
	}
	ret += ".png";
	return ret;
}

int main(int argc, char **argv) {
	--argc;
	++argv;

	if (argc != 2)
		return EXIT_FAILURE;

	constexpr auto &names = magic_enum::enum_names<ID>();
	const int texture_count = names.size() - 1;
	int current_texture = 0;

	std::vector<stbi_uc> combined_texture;
	int texture_size = -1;

	std::vector<bool> combined_transparency;

	for (const auto &i : names) {
		if (i == "kNone")
			continue;
		std::string str = make_texture_filename(i);
		std::cout << i << "->" << str;

		int x, y, c;
		stbi_uc *img = stbi_load(str.c_str(), &x, &y, &c, 4);
		if (img == nullptr) {
			printf("unable to load %s\n", str.c_str());
			return EXIT_FAILURE;
		}
		if (x != y) {
			printf("texture %s (%dx%d) is not a cube\n", str.c_str(), x, y);
			stbi_image_free(img);
			return EXIT_FAILURE;
		}
		if (texture_size == -1) {
			texture_size = x;
			if (x & (x - 1)) {
				printf("texture size (%dx%d) os not power of 2", x, x);
				return EXIT_FAILURE;
			}
			combined_texture.resize(texture_count * texture_size * texture_size * 4);
		} else if (texture_size != x) {
			printf("texture size is not normalized (%s is %dx%d differ from %dx%d)\n", str.c_str(), x, y, texture_size,
			       texture_size);
			stbi_image_free(img);
			return EXIT_FAILURE;
		}
		bool transparent = false;
		if (preserved_opaque_texture.find(current_texture + 1) == preserved_opaque_texture.end())
			for (int p = 0; p < x * x; ++p) {
				if (img[(p << 2) | 3] != 255) {
					transparent = true;
					break;
				}
			}
		combined_transparency.push_back(transparent);
		std::cout << " transparent: " << transparent << std::endl;
		std::copy(img, img + x * x * 4, combined_texture.data() + (current_texture++) * x * x * 4);
	}

	std::vector<unsigned char> png_buffer =
	    WritePngToMemory(texture_size, texture_size * texture_count, combined_texture.data());

	printf("PNG size: %f KB\n", png_buffer.size() / 1024.0f);

	// png content
	std::ofstream output{argv[0]};
	output << bytes_to_array("kBlockTexturePng", png_buffer.data(), png_buffer.size());

	// transparency
	output.close();
	output.open(argv[1]);
	output << "constexpr bool kBlockTextureTransparency[] = { 1, ";
	for (bool b : combined_transparency)
		output << b << ", ";
	output << "};\n";

	return EXIT_SUCCESS;
}