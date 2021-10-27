#ifndef CHUNK_HPP
#define CHUNK_HPP

#include <common/Block.hpp>
#include <common/Light.hpp>
#include <glm/glm.hpp>

#include <memory>
#include <mutex>
#include <vector>

class World;

class Chunk : std::enable_shared_from_this<Chunk> {
public:
	static constexpr uint32_t kSize = 16;

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

	inline const glm::i16vec3 &GetPosition() const { return m_position; }

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
	inline void SetNeighbour(uint32_t idx, const std::weak_ptr<Chunk> &neighbour) { m_neighbours[idx] = neighbour; }
	inline std::shared_ptr<Chunk> LockNeighbour(uint32_t idx) const { return m_neighbours[idx].lock(); }
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
	inline std::shared_ptr<World> LockWorld() const { return m_world.lock(); }

	static inline std::shared_ptr<Chunk> Create(const std::weak_ptr<World> &world, const glm::i16vec3 &position) {
		auto ret = std::make_shared<Chunk>();
		ret->m_position = position;
		ret->m_world = world;
		return ret;
	}

private:
	Block m_blocks[kSize * kSize * kSize];
	Light m_lights[kSize * kSize * kSize];
	glm::i16vec3 m_position;
	std::weak_ptr<Chunk> m_neighbours[26];
	std::weak_ptr<World> m_world;

public:             // Data for rendering
	struct Vertex { // Compressed mesh vertex for chunk
		// x, y, z, face, AO, sunlight, torchlight; texture, u, v
		uint32_t x5_y5_z5_face3_ao2_sl4_tl4, tex8_u5_v5;
		Vertex(uint8_t x, uint8_t y, uint8_t z, BlockFace face, uint8_t ao, LightLvl sunlight, LightLvl torchlight,
		       BlockTexID tex, uint8_t u, uint8_t v)
		    : x5_y5_z5_face3_ao2_sl4_tl4(x | (y << 5u) | (z << 10u) | (face << 15u) | (ao << 18u) | (sunlight << 20u) |
		                                 (torchlight << 24u)),
		      tex8_u5_v5(tex | (u << 8u) | (v << 13u)) {}
	};

	class Mesh {
	private:
		std::vector<Vertex> m_vertices;
		std::vector<uint16_t> m_indices;
		mutable std::mutex m_mutex;

	public:
		const std::vector<Vertex> &GetVertices() const { return m_vertices; }
		const std::vector<uint16_t> &GetIndices() const { return m_indices; }
		std::mutex &GetMutex() const { return m_mutex; }

		friend class ChunkMesher;
	};
	inline Mesh &GetMesh() const { return m_mesh; }

public:
	mutable Mesh m_mesh;
};

#endif
