#ifndef CUBECRAFT3_COMMON_BLOCK_FACE_HPP
#define CUBECRAFT3_COMMON_BLOCK_FACE_HPP

#include <cinttypes>
#include <glm/glm.hpp>

using BlockFace = uint8_t;
struct BlockFaces {
	enum FACE : BlockFace { kRight = 0, kLeft, kTop, kBottom, kFront, kBack };
};

inline constexpr BlockFace BlockFaceOpposite(BlockFace f) { return f ^ 1u; }
template <typename T>
inline constexpr typename std::enable_if<std::is_integral<T>::value, void>::type BlockFaceProceed(T *xyz, BlockFace f) {
	xyz[f >> 1] += 1 - ((f & 1) << 1);
}

template <typename T>
inline constexpr typename std::enable_if<std::is_integral<T>::value, glm::vec<3, T>>::type
BlockFaceProceed(glm::vec<3, T> xyz, BlockFace f) {
	xyz[f >> 1] += 1 - ((f & 1) << 1);
	return xyz;
}

#endif
