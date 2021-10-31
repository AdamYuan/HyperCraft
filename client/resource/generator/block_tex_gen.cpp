#include <common/Block.hpp>

#define MAGIC_ENUM_RANGE_MIN 0
#define MAGIC_ENUM_RANGE_MAX 256
#include <magic_enum.hpp>

#include <cctype>
#include <fstream>
#include <iostream>
#include <vector>

#include <stb_image.h>
#include <stb_image_write.h>

#include "bytes_to_array.hpp"

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

typedef struct {
	int last_pos;
	void *context;
} custom_stbi_mem_context;

static void custom_stbi_write_mem(void *context, void *data, int size) {
	auto *c = (custom_stbi_mem_context *)context;
	char *dst = (char *)c->context;
	char *src = (char *)data;
	int cur_pos = c->last_pos;
	for (int i = 0; i < size; i++) {
		dst[cur_pos++] = src[i];
	}
	c->last_pos = cur_pos;
}

int main(int argc, char **argv) {
	--argc;
	++argv;

	if (argc != 1)
		return EXIT_FAILURE;

	constexpr auto &names = magic_enum::enum_names<BlockTextures::ID>();
	const int texture_count = names.size() - 1;
	int current_texture = 0;

	std::vector<stbi_uc> combined_texture;
	int texture_size = -1;

	for (const auto &i : names) {
		if (i == "kNone")
			continue;
		std::string str = make_texture_filename(i);
		std::cout << i << "->" << str << std::endl;

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
			combined_texture.resize(texture_count * texture_size * texture_size * 4);
		} else if (texture_size != x) {
			printf("texture size is not normalized (%s is %dx%d differ from %dx%d)\n", str.c_str(), x, y, texture_size,
			       texture_size);
			stbi_image_free(img);
			return EXIT_FAILURE;
		}
		std::copy(img, img + x * x * 4, combined_texture.data() + (current_texture++) * x * x * 4);
	}

	std::vector<unsigned char> png_buffer(combined_texture.size());

	custom_stbi_mem_context context;
	context.last_pos = 0;
	context.context = (void *)png_buffer.data();
	int result = stbi_write_png_to_func(custom_stbi_write_mem, &context, texture_size, texture_size * texture_count, 4,
	                                    combined_texture.data(), 0);

	printf("PNG size: %f KB\n", context.last_pos / 1024.0f);

	std::ofstream output{argv[0]};
	output << bytes_to_array("kBlockTexturePng", png_buffer.data(), context.last_pos);

	return EXIT_SUCCESS;
}