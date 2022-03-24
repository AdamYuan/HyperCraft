#ifndef CUBECRAFT3_CLIENT_CHUNK_MESHER_HPP
#define CUBECRAFT3_CLIENT_CHUNK_MESHER_HPP

#include <client/ChunkMesh.hpp>
#include <client/ChunkWorkerBase.hpp>
#include <common/AABB.hpp>
#include <queue>

class ChunkMesher : public ChunkWorkerS26Base {
public:
	static inline std::unique_ptr<ChunkMesher> Create(const std::shared_ptr<Chunk> &chunk_ptr) {
		return std::make_unique<ChunkMesher>(chunk_ptr, false);
	}
	static inline std::unique_ptr<ChunkMesher> CreateWithInitialLight(const std::shared_ptr<Chunk> &chunk_ptr) {
		return std::make_unique<ChunkMesher>(chunk_ptr, true);
	}

	explicit ChunkMesher(const std::shared_ptr<Chunk> &chunk_ptr, bool init_light = false)
	    : ChunkWorkerS26Base(chunk_ptr), m_init_light{init_light} {
		chunk_ptr->PendMeshVersion();
	}
	~ChunkMesher() override = default;

	void Run() override;

private:
	bool m_init_light{};
	struct LightEntry {
		glm::i8vec3 position;
		LightLvl light_lvl;
	};
	class LightQueue {
	private:
		LightEntry m_entries[(kChunkSize + 30) * (kChunkSize + 30) * (kChunkSize + 30)]{};
		LightEntry *m_back = m_entries, *m_top = m_entries;

	public:
		inline void Clear() {
			m_back = m_entries;
			m_top = m_entries;
		}
		inline bool Empty() const { return m_back == m_top; }
		inline LightEntry Pop() { return *(m_back++); }
		inline void Push(const LightEntry &e) { *(m_top++) = e; }
	};

	void initial_sunlight_bfs(Light *light_buffer, std::queue<LightEntry> *queue) const;
	struct AO4 { // compressed ambient occlusion data for 4 vertices (a face)
		uint8_t m_data;
		inline uint8_t Get(uint32_t i) const { return (m_data >> (i << 1u)) & 0x3u; }
		inline void Set(uint32_t i, uint8_t ao) {
			uint8_t mask = ~(0x3u << (i << 1u));
			m_data = (m_data & mask) | (ao << (i << 1u));
		}
		inline uint8_t operator[](uint32_t i) const { return Get(i); }
		inline bool operator==(AO4 r) const { return m_data == r.m_data; }
		inline bool operator!=(AO4 r) const { return m_data != r.m_data; }
	};
	struct Light4 { // compressed light data for 4 vertices (a face)
		Light m_light[4];
		AO4 m_ao;
		void Initialize(BlockFace face, const Block neighbour_blocks[27], const Light neighbour_lights[27]);
		inline bool GetFlip() const {
			return std::abs((int32_t)(m_ao[0] + 1) * (m_light[0].GetSunlight() + 1) -
			                (m_ao[2] + 1) * (m_light[2].GetSunlight() + 1))
			       // + std::max(m_light[0].GetTorchlight(), m_light[2].GetTorchlight())
			       < std::abs((int32_t)(m_ao[1] + 1) * (m_light[1].GetSunlight() + 1) -
			                  (m_ao[3] + 1) * (m_light[3].GetSunlight() + 1));
			// + std::max(m_light[1].GetTorchlight(), m_light[3].GetTorchlight());
		}
		inline bool operator==(Light4 f) const {
			if (m_ao != f.m_ao)
				return false;
			for (uint32_t i = 0; i < 4; ++i)
				if (m_light[i] != f.m_light[i])
					return false;
			return true;
		}
		inline bool operator!=(Light4 f) const {
			if (m_ao != f.m_ao)
				return true;
			for (uint32_t i = 0; i < 4; ++i)
				if (m_light[i] != f.m_light[i])
					return true;
			return false;
		}
	};

	struct MeshGenInfo {
		std::vector<ChunkMeshVertex> vertices;
		std::vector<uint16_t> indices;
		AABB<uint_fast8_t> aabb{};
		bool transparent;
	};

	void generate_face_lights(const Light *light_buffer,
	                          Light4 face_lights[Chunk::kSize * Chunk::kSize * Chunk::kSize][6]) const;

	std::vector<MeshGenInfo>
	generate_mesh(const Light4 face_lights[Chunk::kSize * Chunk::kSize * Chunk::kSize][6]) const;
};

#endif
