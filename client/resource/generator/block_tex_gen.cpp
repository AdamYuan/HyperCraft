#include <common/Block.hpp>

#define MAGIC_ENUM_RANGE_MIN 0
#define MAGIC_ENUM_RANGE_MAX 256
#include <magic_enum.hpp>

#include <cctype>
#include <cstring>
#include <fstream>
#include <iostream>
#include <vector>

#include <png.h>
#include <stb_image.h>

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

// copied from https://gist.github.com/dobrokot/10486786#file-write_png_to_memory_with_libpng-cpp-L25
#define ASSERT_EX(cond, error_message) \
	do { \
		if (!(cond)) { \
			std::cerr << error_message; \
			exit(1); \
		} \
	} while (0)

static void PngWriteCallback(png_structp png_ptr, png_bytep data, png_size_t length) {
	auto *p = (std::vector<unsigned char> *)png_get_io_ptr(png_ptr);
	p->insert(p->end(), data, data + length);
}

struct TPngDestructor {
	png_struct *p;
	explicit TPngDestructor(png_struct *p) : p(p) {}
	~TPngDestructor() {
		if (p) {
			png_destroy_write_struct(&p, nullptr);
		}
	}
};

std::vector<unsigned char> WritePngToMemory(size_t w, size_t h, const unsigned char *data) {
	png_structp p = png_create_write_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
	ASSERT_EX(p, "png_create_write_struct() failed");
	TPngDestructor destroyPng(p);
	png_infop info_ptr = png_create_info_struct(p);
	ASSERT_EX(info_ptr, "png_create_info_struct() failed");
	ASSERT_EX(0 == setjmp(png_jmpbuf(p)), "setjmp(png_jmpbuf(p) failed");
	png_set_IHDR(p, info_ptr, w, h, 8, PNG_COLOR_TYPE_RGBA, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT,
	             PNG_FILTER_TYPE_DEFAULT);
	// png_set_compression_level(p, 1);
	std::vector<unsigned char> ret;
	std::vector<unsigned char *> rows(h);
	for (size_t y = 0; y < h; ++y)
		rows[y] = (unsigned char *)data + y * w * 4;
	png_set_rows(p, info_ptr, &rows[0]);
	png_set_write_fn(p, &ret, PngWriteCallback, nullptr);
	png_write_png(p, info_ptr, PNG_TRANSFORM_IDENTITY, nullptr);
	return ret;
}

int main(int argc, char **argv) {
	--argc;
	++argv;

	if (argc != 2)
		return EXIT_FAILURE;

	constexpr auto &names = magic_enum::enum_names<BlockTextures::ID>();
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