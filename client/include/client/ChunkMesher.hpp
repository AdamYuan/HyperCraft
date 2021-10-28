#ifndef CHUNK_MESHER_HPP
#define CHUNK_MESHER_HPP

#include "ChunkWorker.hpp"

#include <chrono>

class ChunkMesher : public ChunkWorkerS26 {
private:
	std::chrono::time_point<std::chrono::steady_clock> m_starting_time;

public:
	static inline std::unique_ptr<ChunkWorker> Create(const std::weak_ptr<Chunk> &weak_chunk) {
		return std::make_unique<ChunkMesher>(weak_chunk);
	}

	explicit ChunkMesher(const std::weak_ptr<Chunk> &weak_chunk) : ChunkWorkerS26(weak_chunk) {}

	void Run() override;

private:
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
			return (uint32_t)m_ao[0] + m_ao[2] + m_light[1].GetSunlight() + m_light[3].GetSunlight() +
			           std::max(m_light[1].GetTorchlight(), m_light[3].GetTorchlight()) >
			       (uint32_t)m_ao[1] + m_ao[3] + m_light[0].GetSunlight() + m_light[2].GetSunlight() +
			           std::max(m_light[0].GetTorchlight(), m_light[2].GetTorchlight());
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

	void generate_face_lights(Light4 face_lights[Chunk::kSize * Chunk::kSize * Chunk::kSize][6]) const;
	void generate_mesh(const Light4 face_lights[Chunk::kSize * Chunk::kSize * Chunk::kSize][6],
	                   std::vector<Chunk::Vertex> *vertices, std::vector<uint16_t> *indices) const;
	void apply_mesh(std::vector<Chunk::Vertex> &&vertices, std::vector<uint16_t> &&indices) const;
};

#endif
