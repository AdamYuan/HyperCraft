#include <fstream>
#include <iostream>
#include <magic_enum.hpp>

inline static constexpr const char *kBlockCppInFilename = "Block.cpp.in";
inline std::string make_block_hpp_filename(std::string_view x) {
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
	ret += ".hpp";
	return ret;
}

enum ID {
#include "../register/blocks"
};

int main(int argc, char **argv) {
	--argc;
	++argv;

	if (argc != 1)
		return EXIT_FAILURE;

	std::ofstream output{argv[0]};

	constexpr auto &names = magic_enum::enum_names<ID>();
	for (const auto &i : names) {
		std::string filename = make_block_hpp_filename(i);
		output << "#include <" << filename << ">" << std::endl;

		std::cout << i << "->" << filename << std::endl;
	}
	std::ifstream hpp_in{kBlockCppInFilename};
	output << std::string{(std::istreambuf_iterator<char>(hpp_in)), std::istreambuf_iterator<char>()};
}