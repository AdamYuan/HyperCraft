#ifndef WRITE_PNG_HPP
#define WRITE_PNG_HPP

#include <cstring>
#include <iostream>
#include <png.h>
#include <vector>

// copied from https://gist.github.com/dobrokot/10486786#file-write_png_to_memory_with_libpng-cpp-L25
#define ASSERT_EX(cond, error_message) \
	do { \
		if (!(cond)) { \
			std::cout << (error_message); \
			exit(EXIT_FAILURE); \
		} \
	} while (0)

static void PngWriteCallback(png_structp png_ptr, png_bytep data, png_size_t length) {
	auto *p = (std::vector<unsigned char> *)png_get_io_ptr(png_ptr);
	p->insert(p->end(), data, data + length);
}

struct TPngDestructor {
	png_struct *p;
	~TPngDestructor() {
		if (p)
			png_destroy_write_struct(&p, nullptr);
	}
};

std::vector<unsigned char> WritePngToMemory(size_t w, size_t h, const unsigned char *data) {
	png_structp p = png_create_write_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
	ASSERT_EX(p, "png_create_write_struct() failed");
	TPngDestructor png_destructor{p};
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

#undef ASSERT_EX

#endif
