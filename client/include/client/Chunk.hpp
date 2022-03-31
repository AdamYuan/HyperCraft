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
		return ChunkIndex2XYZ(idx, xyz);
	}
	template <typename T>
	static inline constexpr typename std::enable_if<std::is_integral<T>::value, uint32_t>::type XYZ2Index(T x, T y,
	                                                                                                      T z) {
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
		return ::CmpXYZ2NeighbourIndex(cmp_x, cmp_y, cmp_z);
	}
	template <typename T>
	static inline constexpr typename std::enable_if<std::is_integral<T>::value, uint32_t>::type
	GetBlockNeighbourIndex(T x, T y, T z) {
		return GetBlockChunkNeighbourIndex(x, y, z);
	}
	template <typename T>
	static inline constexpr typename std::enable_if<std::is_signed<T>::value && std::is_integral<T>::value, void>::type
	NeighbourIndex2CmpXYZ(uint32_t idx, T *cmp_xyz) {
		return ::NeighbourIndex2CmpXYZ(idx, cmp_xyz);
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
	inline bool NeighbourExpired(uint32_t idx) const { return m_neighbour_weak_ptrs[idx].expired(); }
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

	// Versions
	inline void PendMeshVersion() { m_mesh_version.Pend(); }
	inline void PendLightVersion() { m_light_version.Pend(); }

	inline uint64_t FetchMeshVersion() { return m_mesh_version.Fetch(); }
	inline uint64_t FetchLightVersion() { return m_light_version.Fetch(); }

	inline bool IsLatestMesh() const { return m_mesh_version.IsLatest(); }
	inline bool IsLatestLight() const { return m_light_version.IsLatest(); }

	inline void SwapMesh(uint64_t version, std::vector<std::unique_ptr<ChunkMeshHandle>> &mesh_handles) {
		std::scoped_lock lock{m_mesh_mutex};
		if (!m_mesh_version.Done(version))
			return;
		std::swap(m_mesh_handles, mesh_handles);
	}
	inline void PushLight(uint64_t version, const Light *light_buffer) {
		std::scoped_lock lock{m_light_mutex};
		if (!m_light_version.Done(version))
			return;
		std::copy(light_buffer, light_buffer + kChunkSize * kChunkSize * kChunkSize, m_lights);
	}

	// Creation
	static inline std::shared_ptr<Chunk> Create(const std::weak_ptr<World> &world, const ChunkPos3 &position) {
		auto ret = std::make_shared<Chunk>();
		ret->m_position = position;
		ret->m_world_weak_ptr = world;
		return ret;
	}

	// Mesh Removal
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

	std::mutex m_mesh_mutex, m_light_mutex;
	std::vector<std::unique_ptr<ChunkMeshHandle>> m_mesh_handles;

	template <bool DoneInMutex = false> struct Version {
	private:
		std::atomic_uint64_t m_pending{0}, m_fetched{0}, m_done{0};

	public:
		inline void Pend() { ++m_pending; }
		inline uint64_t Fetch() {
			uint64_t target = m_pending.load(std::memory_order_acquire);

			uint64_t prev = m_fetched.load(std::memory_order_acquire);
			do {
				if (target <= prev)
					return 0;
			} while (
			    !m_fetched.compare_exchange_weak(prev, target, std::memory_order_release, std::memory_order_relaxed));
			return target;
		}
		inline bool Done(uint64_t v) {
			uint64_t target = m_fetched.load(std::memory_order_acquire);
			assert(target >= v); // TODO: remove assert
			if (target > v)
				return false;

			if constexpr (DoneInMutex) {
				uint64_t prev = m_done.load(std::memory_order_acquire);
				if (v > prev) {
					m_done.store(v, std::memory_order_release);
					return true;
				}
			} else {
				uint64_t prev = m_done.load(std::memory_order_acquire);
				do {
					if (prev >= v)
						return false;
				} while (!m_done.compare_exchange_weak(prev, v, std::memory_order_release, std::memory_order_relaxed));
				return true;
			}
			return false;
		}
		inline bool IsLatest() const {
			return m_done.load(std::memory_order_acquire) == m_pending.load(std::memory_order_acquire);
		}
	};
	std::atomic_bool m_generated_flag{}, m_meshed_flag{};
	Version<true> m_mesh_version{}, m_light_version{};
};

#endif
