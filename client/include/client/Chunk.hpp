#ifndef HYPERCRAFT_CLIENT_CHUNK_HPP
#define HYPERCRAFT_CLIENT_CHUNK_HPP

#include <block/Block.hpp>
#include <block/Light.hpp>
#include <common/Position.hpp>
#include <common/Size.hpp>
#include <glm/glm.hpp>

#include "client/mesh/MeshHandle.hpp"
#include <client/ChunkMesh.hpp>

#include <atomic>
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

	// cmp_{x, y, z} = -1, 0, 1, indicating the neighbour's relative position
	template <typename T>
	static inline constexpr typename std::enable_if<std::is_integral<T>::value, uint32_t>::type
	CmpXYZ2NeighbourIndex(T cmp_x, T cmp_y, T cmp_z) {
		return CmpXYZ2NeighbourIndex(cmp_x, cmp_y, cmp_z);
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

	// TODO: Protect Block & Light RW with mutexes
	// Block Getter and Setter
	inline const Block *GetBlockData() const { return m_blocks; }
	template <typename T> inline Block GetBlock(T x, T y, T z) const { return m_blocks[ChunkXYZ2Index(x, y, z)]; }
	template <typename T> inline Block &GetBlockRef(T x, T y, T z) { return m_blocks[ChunkXYZ2Index(x, y, z)]; }
	inline Block GetBlock(uint32_t idx) const { return m_blocks[idx]; }
	inline Block &GetBlockRef(uint32_t idx) { return m_blocks[idx]; }
	template <typename T> inline void SetBlock(T x, T y, T z, Block b) { m_blocks[ChunkXYZ2Index(x, y, z)] = b; }
	inline void SetBlock(uint32_t idx, Block b) { m_blocks[idx] = b; }
	template <std::signed_integral T> inline Block GetBlockFromNeighbour(T x, T y, T z) const {
		return m_blocks[ChunkXYZ2Index((x + kSize) % kSize, (y + kSize) % kSize, (z + kSize) % kSize)];
	}
	template <std::unsigned_integral T> inline Block GetBlockFromNeighbour(T x, T y, T z) const {
		return m_blocks[ChunkXYZ2Index(x % kSize, y % kSize, z % kSize)];
	}

	// Sunlight Getter and Setter
	inline InnerPos1 GetSunlightHeight(uint32_t idx) const { return m_sunlight_heights[idx]; }
	template <typename T> inline InnerPos1 GetSunlightHeight(T x, T z) const {
		return m_sunlight_heights[ChunkXZ2Index(x, z)];
	}
	inline void SetSunlightHeight(uint32_t idx, InnerPos1 h) { m_sunlight_heights[idx] = h; }
	template <typename T> inline void SetSunlightHeight(T x, T z, InnerPos1 h) {
		m_sunlight_heights[ChunkXZ2Index(x, z)] = h;
	}
	template <std::integral T> inline bool GetSunlight(uint32_t idx, T y) const { return y >= m_sunlight_heights[idx]; }
	template <typename T> inline bool GetSunlight(T x, T y, T z) const {
		return y >= m_sunlight_heights[ChunkXZ2Index(x, z)];
	}
	template <std::signed_integral T> inline bool GetSunlightFromNeighbour(T x, T y, T z) const {
		return GetSunlight((x + kSize) % kSize, (y + kSize) % kSize, (z + kSize) % kSize);
	}
	template <std::unsigned_integral T> inline Block GetSunlightFromNeighbour(T x, T y, T z) const {
		return GetSunlight(x % kSize, y % kSize, z % kSize);
	}

	// Creation
	inline explicit Chunk(const ChunkPos3 &position) : m_position{position} {}
	static inline std::shared_ptr<Chunk> Create(const ChunkPos3 &position) { return std::make_shared<Chunk>(position); }

	// Generated Flag
	inline void SetGeneratedFlag() { m_generated_flag.store(true, std::memory_order_release); }
	inline bool IsGenerated() const { return m_generated_flag.load(std::memory_order_acquire); }

private:
	const ChunkPos3 m_position{};

	Block m_blocks[kSize * kSize * kSize];
	InnerPos1 m_sunlight_heights[kSize * kSize]{};
	std::atomic_bool m_generated_flag{false};
};

} // namespace hc::client

#endif
