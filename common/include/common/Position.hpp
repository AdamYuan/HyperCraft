//
// Created by adamyuan on 1/25/22.
//

#ifndef HYPERCRAFT_COMMON_POSITION_HPP
#define HYPERCRAFT_COMMON_POSITION_HPP

#include <cinttypes>
#include <glm/glm.hpp>
#include <glm/gtx/hash.hpp>

#include "Size.hpp"

namespace hc {

using ChunkPos1 = int16_t;
using ChunkPos2 = glm::vec<2, ChunkPos1>;
using ChunkPos3 = glm::vec<3, ChunkPos1>;

template <typename T>
static inline constexpr typename std::enable_if<std::is_integral<T>::value, void>::type ChunkIndex2XYZ(uint32_t idx,
                                                                                                       T *xyz) {
	xyz[0] = idx % kChunkSize;
	idx /= kChunkSize;
	xyz[2] = idx % kChunkSize;
	xyz[1] = idx / kChunkSize;
}
template <typename T>
static inline constexpr typename std::enable_if<std::is_integral<T>::value, uint32_t>::type ChunkXYZ2Index(T x, T y,
                                                                                                           T z) {
	return x + (y * kChunkSize + z) * kChunkSize;
}
template <typename T>
static inline constexpr typename std::enable_if<std::is_signed<T>::value && std::is_integral<T>::value, bool>::type
IsValidChunkPosition(T x, T y, T z) {
	return x >= 0 && x < (T)kChunkSize && y >= 0 && y < (T)kChunkSize && z >= 0 && z < (T)kChunkSize;
}
template <typename T>
static inline constexpr typename std::enable_if<std::is_unsigned<T>::value, bool>::type IsValidChunkPosition(T x, T y,
                                                                                                             T z) {
	return x <= kChunkSize && y <= kChunkSize && z <= kChunkSize;
}

// cmp_{x, y, z} = -1, 0, 1, indicating the neighbour's relative position
template <typename T>
static inline constexpr typename std::enable_if<std::is_integral<T>::value, uint32_t>::type
CmpXYZ2NeighbourIndex(T cmp_x, T cmp_y, T cmp_z) {
	constexpr uint32_t kLookUp[3] = {1, 2, 0};
	return kLookUp[cmp_x + 1] * 9u + kLookUp[cmp_y + 1] * 3u + kLookUp[cmp_z + 1];
}
template <typename T>
static inline constexpr typename std::enable_if<std::is_integral<T>::value, uint32_t>::type
GetBlockChunkNeighbourIndex(T x, T y, T z) {
	return CmpXYZ2NeighbourIndex(x < 0 ? -1 : (x >= (T)kChunkSize ? 1 : 0), y < 0 ? -1 : (y >= (T)kChunkSize ? 1 : 0),
	                             z < 0 ? -1 : (z >= (T)kChunkSize ? 1 : 0));
}
template <typename T>
static inline constexpr typename std::enable_if<std::is_signed<T>::value && std::is_integral<T>::value, void>::type
NeighbourIndex2CmpXYZ(uint32_t idx, T *cmp_xyz) {
	constexpr T kRevLookUp[3] = {1, -1, 0};
	cmp_xyz[2] = kRevLookUp[idx % 3u];
	idx /= 3u;
	cmp_xyz[1] = kRevLookUp[idx % 3u];
	cmp_xyz[0] = kRevLookUp[idx / 3u];
}

} // namespace hc

#endif
