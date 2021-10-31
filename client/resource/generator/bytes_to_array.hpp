#ifndef BYTES_TO_ARRAY_HPP
#define BYTES_TO_ARRAY_HPP

#include <sstream>
#include <string>

inline std::string bytes_to_array(const char *array_name, unsigned char *data, size_t size) {
	std::stringstream ss;
	std::string ret = "constexpr unsigned char ";
	ret += array_name;
	ret += "[] = {\n";

	for (size_t i = 0; i < size; ++i) {
		ss << (int)data[i] << ',';
		if (ss.str().length() >= 80) {
			ret += '\t' + ss.str() + '\n';
			ss.str("");
		}
	}
	ret += '\t' + ss.str() + "\n};";
	return ret;
}

#endif
