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

using BlockPos1 = int32_t;
using BlockPos2 = glm::vec<2, BlockPos1>;
using BlockPos3 = glm::vec<3, BlockPos1>;

using InnerPos1 = int8_t;
using InnerPos2 = glm::vec<2, InnerPos1>;
using InnerPos3 = glm::vec<3, InnerPos1>;

using InnerIndex2 = uint16_t;
using InnerIndex3 = uint16_t;

static_assert(std::numeric_limits<InnerPos1>::max() >= kChunkSize);

using ChunkPos1 = int16_t;
using ChunkPos2 = glm::vec<2, ChunkPos1>;
using ChunkPos3 = glm::vec<3, ChunkPos1>;

static inline constexpr ChunkPos1 ChunkPosFromBlockPos(BlockPos1 p) {
	BlockPos1 cmp1 = p < 0;
	return ChunkPos1((p + cmp1) / (BlockPos1)kChunkSize - cmp1);
}
static inline constexpr ChunkPos2 ChunkPosFromBlockPos(const BlockPos2 &p) {
	auto cmp2 = (BlockPos2)glm::lessThan(p, BlockPos2{0, 0});
	return (p + cmp2) / (BlockPos1)kChunkSize - cmp2;
}
static inline constexpr ChunkPos3 ChunkPosFromBlockPos(const BlockPos3 &p) {
	auto cmp3 = (BlockPos3)glm::lessThan(p, BlockPos3{0, 0, 0});
	return (p + cmp3) / (BlockPos1)kChunkSize - cmp3;
}
static inline constexpr InnerPos1 InnerPosFromBlockPos(BlockPos1 p) {
	return InnerPos1(p - (BlockPos1)ChunkPosFromBlockPos(p) * (BlockPos1)kChunkSize);
}
static inline constexpr InnerPos2 InnerPosFromBlockPos(const BlockPos2 &p) {
	return p - (BlockPos2)ChunkPosFromBlockPos(p) * (BlockPos1)kChunkSize;
}
static inline constexpr InnerPos3 InnerPosFromBlockPos(BlockPos3 p) {
	return p - (BlockPos3)ChunkPosFromBlockPos(p) * (BlockPos1)kChunkSize;
}
static inline constexpr std::pair<ChunkPos1, InnerPos1> ChunkInnerPosFromBlockPos(BlockPos1 p) {
	auto chunk_pos = ChunkPosFromBlockPos(p);
	return {chunk_pos, InnerPos1(p - (BlockPos1)chunk_pos * (BlockPos1)kChunkSize)};
}
static inline constexpr std::pair<ChunkPos2, InnerPos2> ChunkInnerPosFromBlockPos(BlockPos2 p) {
	auto chunk_pos = ChunkPosFromBlockPos(p);
	return {chunk_pos, InnerPos2(p - (BlockPos2)chunk_pos * (BlockPos1)kChunkSize)};
}
static inline constexpr std::pair<ChunkPos3, InnerPos3> ChunkInnerPosFromBlockPos(BlockPos3 p) {
	auto chunk_pos = ChunkPosFromBlockPos(p);
	return {chunk_pos, InnerPos3(p - (BlockPos3)chunk_pos * (BlockPos1)kChunkSize)};
}

static inline constexpr uint32_t ChunkPosLength2(ChunkPos1 p) { return p * p; }
static inline constexpr uint32_t ChunkPosDistance2(ChunkPos1 l, ChunkPos1 r) {
	return ChunkPosLength2(ChunkPos1(l - r));
}
static inline constexpr uint32_t ChunkPosLength2(const ChunkPos2 &p) {
	return (uint32_t)p.x * (uint32_t)p.x + (uint32_t)p.y * (uint32_t)p.y;
}
static inline constexpr uint32_t ChunkPosDistance2(const ChunkPos2 &l, const ChunkPos2 &r) {
	return ChunkPosLength2(l - r);
}
static inline constexpr uint32_t ChunkPosLength2(const ChunkPos3 &p) {
	return (uint32_t)p.x * (uint32_t)p.x + (uint32_t)p.y * (uint32_t)p.y + (uint32_t)p.z * (uint32_t)p.z;
}
static inline constexpr uint32_t ChunkPosDistance2(const ChunkPos3 &l, const ChunkPos3 &r) {
	return ChunkPosLength2(l - r);
}

/* template <std::integral T> static inline constexpr void InnerPos3FromIndex(InnerIndex3 idx, T *xyz) {
    xyz[0] = idx % kChunkSize;
    idx /= kChunkSize;
    xyz[2] = idx % kChunkSize;
    xyz[1] = idx / kChunkSize;
} */
template <std::integral T = InnerPos1> static inline constexpr glm::vec<3, T> InnerPos3FromIndex(InnerIndex3 idx) {
	glm::vec<3, T> xyz;
	xyz[0] = idx % kChunkSize;
	idx /= kChunkSize;
	xyz[2] = idx % kChunkSize;
	xyz[1] = idx / kChunkSize;
	return xyz;
}
template <std::integral T> static inline constexpr InnerIndex3 InnerIndex3FromPos(T x, T y, T z) {
	return x + (y * kChunkSize + z) * kChunkSize;
}

template <std::integral T> static inline constexpr InnerIndex3 InnerIndex3FromPos(const glm::vec<3, T> &xyz) {
	return xyz[0] + (xyz[1] * kChunkSize + xyz[2]) * kChunkSize;
}

/* template <std::integral T> static inline constexpr void InnerPos2FromIndex(InnerIndex2 idx, T *xz) {
    xz[0] = idx % kChunkSize;
    xz[1] = idx / kChunkSize;
} */
template <std::integral T = InnerPos1> static inline constexpr glm::vec<2, T> InnerPos2FromIndex(InnerIndex2 idx) {
	glm::vec<2, T> xz;
	xz[0] = idx % kChunkSize;
	xz[1] = idx / kChunkSize;
	return xz;
}
template <std::integral T = InnerPos1> static inline constexpr InnerIndex2 InnerIndex2FromPos(T x, T z) {
	return x + z * kChunkSize;
}
template <std::integral T = InnerPos1>
static inline constexpr InnerIndex2 InnerIndex2FromPos(const glm::vec<2, T> &xz) {
	return xz[0] + xz[1] * kChunkSize;
}

template <std::signed_integral T> static inline constexpr bool IsValidInnerPos(T x, T y, T z) {
	return x >= 0 && x < (T)kChunkSize && y >= 0 && y < (T)kChunkSize && z >= 0 && z < (T)kChunkSize;
}
template <std::unsigned_integral T> static inline constexpr bool IsValidInnerPos(T x, T y, T z) {
	return x <= kChunkSize && y <= kChunkSize && z <= kChunkSize;
}
template <std::signed_integral T> static inline constexpr bool IsValidInnerPos(T x, T z) {
	return x >= 0 && x < (T)kChunkSize && z >= 0 && z < (T)kChunkSize;
}
template <std::unsigned_integral T> static inline constexpr bool IsValidInnerPos(T x, T z) {
	return x <= kChunkSize && z <= kChunkSize;
}

// cmp_{x, y, z} = -1, 0, 1, indicating the neighbour's relative position
template <std::integral T> static inline constexpr uint32_t CmpXYZ2NeighbourIndex(T cmp_x, T cmp_y, T cmp_z) {
	constexpr uint32_t kLookUp[3] = {1, 2, 0};
	return kLookUp[cmp_x + 1] * 9u + kLookUp[cmp_y + 1] * 3u + kLookUp[cmp_z + 1];
}
template <std::integral T> static inline constexpr uint32_t GetBlockChunkNeighbourIndex(T x, T y, T z) {
	return CmpXYZ2NeighbourIndex(x < 0 ? -1 : (x >= (T)kChunkSize ? 1 : 0), y < 0 ? -1 : (y >= (T)kChunkSize ? 1 : 0),
	                             z < 0 ? -1 : (z >= (T)kChunkSize ? 1 : 0));
}
template <std::signed_integral T> static inline constexpr void NeighbourIndex2CmpXYZ(uint32_t idx, T *cmp_xyz) {
	constexpr T kRevLookUp[3] = {1, -1, 0};
	cmp_xyz[2] = kRevLookUp[idx % 3u];
	idx /= 3u;
	cmp_xyz[1] = kRevLookUp[idx % 3u];
	cmp_xyz[0] = kRevLookUp[idx / 3u];
}

} // namespace hc

#endif
