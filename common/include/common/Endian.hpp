#ifndef CUBECRAFT3_COMMON_ENDIAN_HPP
#define CUBECRAFT3_COMMON_ENDIAN_HPP

#ifndef IS_BIG_ENDIAN
#define IS_SMALL_ENDIAN
#endif

constexpr bool is_big_endian() {
#ifdef IS_BIG_ENDIAN
	return true;
#else
	return false;
#endif
}
constexpr bool is_small_endian() {
#ifdef IS_SMALL_ENDIAN
	return true;
#else
	return false;
#endif
}

#endif
