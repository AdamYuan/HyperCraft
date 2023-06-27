#define MAGIC_ENUM_RANGE_MIN 0
#define MAGIC_ENUM_RANGE_MAX 256
#include <magic_enum.hpp>

constexpr int kTextureSize = 16;
constexpr int kTextureLength = kTextureSize * kTextureSize;

#include <bitset>
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
#include "../register/block_textures"
};

std::set<uint32_t> transparent_pass_textures = {
    ID::kWater, ID::kAcaciaLeaves, ID::kBirchLeaves, ID::kJungleLeaves, ID::kSpruceLeaves,  ID::kOakLeaves,
    ID::kIce,   ID::kGrassBoreal,  ID::kGrassPlain,  ID::kGrassSavanna, ID::kGrassTropical,
};
std::set<uint32_t> liquid_textures = {ID::kWater};

inline std::string make_texture_filename(std::string_view x) {
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

class Texture {
private:
	int m_id{};
	stbi_uc *m_bytes{};
	bool m_transparent{};
	// std::bitset<kTextureLength> m_opaque_mask{};

public:
	Texture() { m_transparent = true; }
	Texture(int id, stbi_uc *bytes) : m_id{id}, m_bytes{bytes} {
		m_transparent = false;
		for (int p = 0; p < kTextureLength; ++p)
			if (m_bytes[(p << 2) | 3] != 255) {
				m_transparent = true;
				break;
			}
	}
	inline const stbi_uc *GetBytes() const { return m_bytes; }
	inline int GetID() const { return m_id; }
	inline bool IsEmpty() const { return m_id == 0; }
	inline bool IsTransparent() const { return m_transparent; }
	inline bool UseTransPass() const {
		return IsTransparent() && transparent_pass_textures.find(m_id) != transparent_pass_textures.end();
	}
	inline bool IsLiquid() const { return liquid_textures.find(m_id) != liquid_textures.end(); }
	// inline bool Occupy(const Texture &r) const { return (m_opaque_mask | r.m_opaque_mask) == m_opaque_mask; }
};

// TODO: precompute texture coverage table (to cull unnecessary faces)
int main(int argc, char **argv) {
	--argc;
	++argv;

	if (argc != 2)
		return EXIT_FAILURE;

	constexpr auto &names = magic_enum::enum_names<ID>();
	const int texture_count = names.size() - 1;
	int current_texture = 0;

	std::vector<Texture> textures;
	textures.emplace_back();
	std::vector<stbi_uc> combined_texture(kTextureLength * 4 * texture_count);

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
			return EXIT_FAILURE;
		}
		if (x != kTextureSize) {
			printf("texture size (%dx%d) is not %dx%d", x, x, kTextureSize, kTextureSize);
			return EXIT_FAILURE;
		}
		textures.emplace_back(current_texture + 1, img);
		auto &tex = textures.back();
		std::cout << std::endl;
		std::copy(img, img + x * x * 4, combined_texture.data() + (current_texture++) * x * x * 4);
	}

	std::vector<unsigned char> png_buffer =
	    WritePngToMemory(kTextureSize, kTextureSize * texture_count, combined_texture.data());

	printf("PNG size: %f KB\n", png_buffer.size() / 1024.0f);

	// png content
	std::ofstream output{argv[0]};
	output << bytes_to_array("kBlockTexturePng", png_buffer.data(), png_buffer.size());

	// transparency
	output.close();
	output.open(argv[1]);
	output << "constexpr bool kBlockTextureTransparency[] = {";
	for (const auto &t : textures)
		output << t.IsTransparent() << ",";
	output << "};\n";

	output << "constexpr bool kBlockTextureTransPass[] = {";
	for (const auto &t : textures)
		output << t.UseTransPass() << ",";
	output << "};\n";

	output << "constexpr bool kBlockTextureLiquid[] = {";
	for (const auto &t : textures)
		output << t.IsLiquid() << ",";
	output << "};\n";

	return EXIT_SUCCESS;
}