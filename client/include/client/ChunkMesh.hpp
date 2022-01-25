#ifndef CUBECRAFT3_COMMON_CHUNK_MESH_HPP
#define CUBECRAFT3_COMMON_CHUNK_MESH_HPP

#include <client/Config.hpp>

#include <common/Block.hpp>
#include <common/Light.hpp>
#include <common/Position.hpp>

#include <myvk/Buffer.hpp>
#include <myvk/CommandBuffer.hpp>

#include <atomic_shared_ptr.hpp>

class Chunk;

class ChunkMesh : public std::enable_shared_from_this<ChunkMesh> {
public:
	// Data for rendering
	struct Vertex { // Compressed mesh vertex for chunk
		// x, y, z, face, AO, sunlight, torchlight; resource, u, v
		uint32_t x5_y5_z5_face3_ao2_sl4_tl4, tex8_u5_v5;
		Vertex(uint8_t x, uint8_t y, uint8_t z, BlockFace face, uint8_t ao, LightLvl sunlight, LightLvl torchlight,
		       BlockTexID tex, uint8_t u, uint8_t v)
		    : x5_y5_z5_face3_ao2_sl4_tl4(x | (y << 5u) | (z << 10u) | (face << 15u) | (ao << 18u) | (sunlight << 20u) |
		                                 (torchlight << 24u)),
		      tex8_u5_v5((tex - 1u) | (u << 8u) | (v << 13u)) {}
	};
	struct UpdateInfo {
		std::vector<Vertex> vertices;
		std::vector<uint16_t> indices;
	};

private:
	std::weak_ptr<Chunk> m_chunk_weak_ptr;

	std::atomic_shared_ptr<myvk::Buffer> m_buffer;
	std::shared_ptr<myvk::Buffer> m_frame_buffers[kFrameCount];

public:
	static std::shared_ptr<ChunkMesh> Allocate(const std::shared_ptr<Chunk> &chunk_ptr);

	void Update(const UpdateInfo &update_info);
	bool CmdDraw(const std::shared_ptr<myvk::CommandBuffer> &command_buffer,
	             const std::shared_ptr<myvk::PipelineLayout> &pipeline_layout, const ChunkPos3 &chunk_pos,
	             uint32_t frame); // if the mesh can be destroyed, return true

	inline const std::weak_ptr<Chunk> &GetChunkWeakPtr() const { return m_chunk_weak_ptr; }
	inline std::shared_ptr<Chunk> LockChunk() const { return m_chunk_weak_ptr.lock(); }
};

#endif
