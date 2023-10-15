#ifndef HYPERCRAFT_CLIENT_CHUNK_HPP
#define HYPERCRAFT_CLIENT_CHUNK_HPP

#include <block/Block.hpp>
#include <block/Light.hpp>
#include <common/Position.hpp>
#include <common/Size.hpp>
#include <glm/glm.hpp>

#include "client/mesh/MeshHandle.hpp"
#include <client/ChunkMesh.hpp>

#include <chrono>
#include <memory>
#include <vector>

namespace hc::client {

class World;

class Chunk : public std::enable_shared_from_this<Chunk> {
private:
	using Block = block::Block;
	using Light = block::Light;

public:
	static constexpr uint32_t kSize = kChunkSize;

	template <typename T, typename = std::enable_if_t<std::is_integral_v<T>>>
	static inline constexpr void Index2XYZ(uint32_t idx, T *xyz) {
		return ChunkIndex2XYZ(idx, xyz);
	}
	template <typename T, typename = std::enable_if_t<std::is_integral_v<T>>>
	static inline constexpr uint32_t XYZ2Index(T x, T y, T z) {
		return ChunkXYZ2Index(x, y, z);
	}
	template <typename T>
	static inline constexpr typename std::enable_if<std::is_signed<T>::value && std::is_integral<T>::value, bool>::type
	IsValidPosition(T x, T y, T z) {
		return IsValidChunkPosition(x, y, z);
	}
	template <typename T>
	static inline constexpr typename std::enable_if<std::is_unsigned<T>::value, bool>::type IsValidPosition(T x, T y,
	                                                                                                        T z) {
		return IsValidChunkPosition(x, y, z);
	}

	// cmp_{x, y, z} = -1, 0, 1, indicating the neighbour's relative position
	template <typename T>
	static inline constexpr typename std::enable_if<std::is_integral<T>::value, uint32_t>::type
	CmpXYZ2NeighbourIndex(T cmp_x, T cmp_y, T cmp_z) {
		return hc::CmpXYZ2NeighbourIndex(cmp_x, cmp_y, cmp_z);
	}
	template <typename T>
	static inline constexpr typename std::enable_if<std::is_integral<T>::value, uint32_t>::type
	GetBlockNeighbourIndex(T x, T y, T z) {
		return GetBlockChunkNeighbourIndex(x, y, z);
	}
	template <typename T>
	static inline constexpr typename std::enable_if<std::is_signed<T>::value && std::is_integral<T>::value, void>::type
	NeighbourIndex2CmpXYZ(uint32_t idx, T *cmp_xyz) {
		return hc::NeighbourIndex2CmpXYZ(idx, cmp_xyz);
	}

	inline const ChunkPos3 &GetPosition() const { return m_position; }

	// Block Getter and Setter
	inline const Block *GetBlockData() const { return m_blocks; }
	template <typename T> inline Block GetBlock(T x, T y, T z) const { return m_blocks[XYZ2Index(x, y, z)]; }
	template <typename T> inline Block &GetBlockRef(T x, T y, T z) { return m_blocks[XYZ2Index(x, y, z)]; }
	inline Block GetBlock(uint32_t idx) const { return m_blocks[idx]; }
	inline Block &GetBlockRef(uint32_t idx) { return m_blocks[idx]; }
	template <typename T> inline void SetBlock(T x, T y, T z, Block b) { m_blocks[XYZ2Index(x, y, z)] = b; }
	inline void SetBlock(uint32_t idx, Block b) { m_blocks[idx] = b; }

	// Light Getter and Setter
	inline const Light *GetLightData() const { return m_lights; }
	template <typename T>
	inline typename std::enable_if<std::is_integral<T>::value, Light>::type GetLight(T x, T y, T z) const {
		return m_lights[XYZ2Index(x, y, z)];
	}
	template <typename T>
	inline typename std::enable_if<std::is_integral<T>::value, Light &>::type GetLightRef(T x, T y, T z) {
		return m_lights[XYZ2Index(x, y, z)];
	}
	inline Light GetLight(uint32_t idx) const { return m_lights[idx]; }
	inline Light &GetLightRef(uint32_t idx) { return m_lights[idx]; }
	template <typename T>
	inline typename std::enable_if<std::is_integral<T>::value, void>::type SetLight(T x, T y, T z, Light l) {
		m_lights[XYZ2Index(x, y, z)] = l;
	}
	inline void SetLight(uint32_t idx, Light l) { m_lights[idx] = l; }

	// Neighbours
	template <typename T>
	inline typename std::enable_if<std::is_integral<T>::value && std::is_signed<T>::value, Block>::type
	GetBlockFromNeighbour(T x, T y, T z) const {
		return m_blocks[XYZ2Index((x + kSize) % kSize, (y + kSize) % kSize, (z + kSize) % kSize)];
	}
	template <typename T, typename = std::enable_if_t<std::is_unsigned_v<T>>>
	inline Block GetBlockFromNeighbour(T x, T y, T z) const {
		return m_blocks[XYZ2Index(x % kSize, y % kSize, z % kSize)];
	}
	template <typename T>
	inline typename std::enable_if<std::is_integral<T>::value && std::is_signed<T>::value, Light>::type
	GetLightFromNeighbour(T x, T y, T z) const {
		return m_lights[XYZ2Index((x + kSize) % kSize, (y + kSize) % kSize, (z + kSize) % kSize)];
	}
	template <typename T>
	inline typename std::enable_if<std::is_unsigned<T>::value, Light>::type GetLightFromNeighbour(T x, T y, T z) const {
		return m_lights[XYZ2Index(x % kSize, y % kSize, z % kSize)];
	}

	// Creation
	static inline std::shared_ptr<Chunk> Create(const ChunkPos3 &position) {
		auto ret = std::make_shared<Chunk>();
		ret->m_position = position;
		return ret;
	}

private:
	Block m_blocks[kSize * kSize * kSize];
	Light m_lights[kSize * kSize * kSize];

	ChunkPos3 m_position{};
};

} // namespace hc::client

#endif
