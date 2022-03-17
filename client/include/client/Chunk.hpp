#ifndef CUBECRAFT3_CLIENT_CHUNK_HPP
#define CUBECRAFT3_CLIENT_CHUNK_HPP

#include <common/Block.hpp>
#include <common/Light.hpp>
#include <common/Position.hpp>
#include <common/Size.hpp>
#include <glm/glm.hpp>

#include <client/ChunkMesh.hpp>
#include <client/MeshHandle.hpp>

#include <chrono>
#include <memory>
#include <vector>

class World;

class Chunk : public std::enable_shared_from_this<Chunk> {
public:
	static constexpr uint32_t kSize = kChunkSize;

	template <typename T>
	static inline constexpr typename std::enable_if<std::is_integral<T>::value, void>::type Index2XYZ(uint32_t idx,
	                                                                                                  T *xyz) {
		xyz[0] = idx % kSize;
		idx /= kSize;
		xyz[2] = idx % kSize;
		xyz[1] = idx / kSize;
	}
	template <typename T>
	static inline constexpr typename std::enable_if<std::is_integral<T>::value, uint32_t>::type XYZ2Index(T x, T y,
	                                                                                                      T z) {
		return x + (y * kSize + z) * kSize;
	}
	template <typename T>
	static inline constexpr typename std::enable_if<std::is_signed<T>::value && std::is_integral<T>::value, bool>::type
	IsValidPosition(T x, T y, T z) {
		return x >= 0 && x < kSize && y >= 0 && y < kSize && z >= 0 && z < kSize;
	}
	template <typename T>
	static inline constexpr typename std::enable_if<std::is_unsigned<T>::value, bool>::type IsValidPosition(T x, T y,
	                                                                                                        T z) {
		return x <= kSize && y <= kSize && z <= kSize;
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
	GetBlockNeighbourIndex(T x, T y, T z) {
		return CmpXYZ2NeighbourIndex(x < 0 ? -1 : (x >= kSize ? 1 : 0), y < 0 ? -1 : (y >= kSize ? 1 : 0),
		                             z < 0 ? -1 : (z >= kSize ? 1 : 0));
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

	inline const ChunkPos3 &GetPosition() const { return m_position; }

	// Block Getter and Setter
	inline const Block *GetBlockData() const { return m_blocks; }
	template <typename T>
	inline typename std::enable_if<std::is_integral<T>::value, Block>::type GetBlock(T x, T y, T z) const {
		return m_blocks[XYZ2Index(x, y, z)];
	}
	template <typename T>
	inline typename std::enable_if<std::is_integral<T>::value, Block &>::type GetBlockRef(T x, T y, T z) {
		return m_blocks[XYZ2Index(x, y, z)];
	}
	inline Block GetBlock(uint32_t idx) const { return m_blocks[idx]; }
	inline Block &GetBlockRef(uint32_t idx) { return m_blocks[idx]; }
	template <typename T>
	inline typename std::enable_if<std::is_integral<T>::value, void>::type SetBlock(T x, T y, T z, Block b) {
		m_blocks[XYZ2Index(x, y, z)] = b;
	}
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
	inline void SetNeighbour(uint32_t idx, const std::weak_ptr<Chunk> &neighbour) {
		m_neighbour_weak_ptrs[idx] = neighbour;
	}
	inline std::shared_ptr<Chunk> LockNeighbour(uint32_t idx) const { return m_neighbour_weak_ptrs[idx].lock(); }
	template <typename T>
	inline typename std::enable_if<std::is_integral<T>::value && std::is_signed<T>::value, Block>::type
	GetBlockFromNeighbour(T x, T y, T z) const {
		return m_blocks[XYZ2Index((x + kSize) % kSize, (y + kSize) % kSize, (z + kSize) % kSize)];
	}
	template <typename T>
	inline typename std::enable_if<std::is_unsigned<T>::value, Block>::type GetBlockFromNeighbour(T x, T y, T z) const {
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

	// World (parent)
	inline std::shared_ptr<World> LockWorld() const { return m_world_weak_ptr.lock(); }

	// Flags
	bool IsGenerated() const { return m_generated_flag.load(std::memory_order_acquire); }
	void SetGeneratedFlag() { m_generated_flag.store(true, std::memory_order_release); }
	bool IsMeshed() const { return m_meshed_flag.load(std::memory_order_acquire); }
	void SetMeshedFlag() { m_meshed_flag.store(true, std::memory_order_release); }

	// Creation
	static inline std::shared_ptr<Chunk> Create(const std::weak_ptr<World> &world, const ChunkPos3 &position) {
		auto ret = std::make_shared<Chunk>();
		ret->m_position = position;
		ret->m_world_weak_ptr = world;
		return ret;
	}

	// Mesh Removal
	inline void SetMeshes(std::vector<std::unique_ptr<ChunkMeshHandle>> &&mesh_handles) {
		std::scoped_lock lock{m_mesh_mutex};
		m_mesh_handles = std::move(mesh_handles);
	}
	inline std::vector<std::unique_ptr<ChunkMeshHandle>> &&MoveMeshes() {
		std::scoped_lock lock{m_mesh_mutex};
		return std::move(m_mesh_handles);
	}
	inline void SetMeshFinalize() const {
		for (const auto &i : m_mesh_handles)
			i->SetFinalize();
	}

private:
	Block m_blocks[kSize * kSize * kSize];
	Light m_lights[kSize * kSize * kSize];

	ChunkPos3 m_position{};

	std::weak_ptr<Chunk> m_neighbour_weak_ptrs[26];
	std::weak_ptr<World> m_world_weak_ptr;

	std::mutex m_mesh_mutex;
	std::vector<std::unique_ptr<ChunkMeshHandle>> m_mesh_handles;
	friend class ChunkMesher;

	std::atomic_bool m_generated_flag{}, m_meshed_flag{};
};

#endif
