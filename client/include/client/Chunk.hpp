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

	// World (parent)
	inline std::shared_ptr<World> LockWorld() const { return m_world_weak_ptr.lock(); }

	// Initial Flags
	bool IsGenerated() const { return m_initial_generated_flag.load(std::memory_order_acquire); }
	void SetGeneratedFlag() { m_initial_generated_flag.store(true, std::memory_order_release); }
	bool IsMeshed() const { return m_initial_meshed_flag.load(std::memory_order_acquire); }
	void SetMeshedFlag() { m_initial_meshed_flag.store(true, std::memory_order_release); }

	// Sync
	inline auto &GetMeshSync() { return m_mesh_sync; }
	inline auto &GetLightSync() { return m_light_sync; }

	inline void PushMesh(uint64_t version, std::vector<std::unique_ptr<ChunkMeshHandle>> &mesh_handles) {
		m_mesh_sync.Done(version, [this, &mesh_handles]() { m_mesh_handles = std::move(mesh_handles); });
	}
	inline void PushLight(uint64_t version, const Light *light_buffer) {
		m_light_sync.Done(version, [this, light_buffer]() {
			std::copy(light_buffer, light_buffer + kChunkSize * kChunkSize * kChunkSize, m_lights);
		});
	}

	// Creation
	static inline std::shared_ptr<Chunk> Create(const std::weak_ptr<World> &world, const ChunkPos3 &position) {
		auto ret = std::make_shared<Chunk>();
		ret->m_position = position;
		ret->m_world_weak_ptr = world;
		return ret;
	}

	// Finalize
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

	std::vector<std::unique_ptr<ChunkMeshHandle>> m_mesh_handles;

	struct Sync {
	private:
		std::atomic_bool m_pending{};
		std::atomic_uint64_t m_version{};
		std::mutex m_done_mutex;

	public:
		inline bool IsPending() { return m_pending.load(std::memory_order_acquire); }
		inline void Pend() { m_pending.store(true, std::memory_order_release); }
		inline void Cancel() { m_pending.store(false, std::memory_order_release); }
		inline uint64_t FetchVersion() {
			m_pending.store(false, std::memory_order_release);
			return m_version.fetch_add(1, std::memory_order_acq_rel) + 1;
		}
		template <typename DoneFunc> inline bool Done(uint64_t version, DoneFunc &&done_func) {
			std::scoped_lock lock{m_done_mutex};
			if (version == m_version.load(std::memory_order_acquire)) {
				done_func();
				return true;
			}
			return false;
		}
	};
	std::atomic_bool m_initial_generated_flag{false}, m_initial_meshed_flag{false};
	Sync m_mesh_sync{}, m_light_sync{};
};

} // namespace hc::client

#endif
