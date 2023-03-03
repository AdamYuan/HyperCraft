#ifndef CUBECRAFT3_CLIENT_CHUNK_RENDERER_HPP
#define CUBECRAFT3_CLIENT_CHUNK_RENDERER_HPP

#include <cinttypes>

#include <client/ChunkMesh.hpp>
#include <client/MeshPool.hpp>
#include <common/AABB.hpp>
#include <common/Position.hpp>

class ChunkMeshPool : public ChunkMeshPoolBase {
private:
	inline static constexpr uint32_t kClusterFaceCount = 4 * 1024 * 1024;
	inline static constexpr uint32_t kLog2MaxMeshesPerCluster = 15u;

public:
	explicit ChunkMeshPool(const myvk::Ptr<myvk::Device> &device)
	    : ChunkMeshPoolBase(device, kClusterFaceCount * 4 * sizeof(ChunkMeshVertex),
	                        kClusterFaceCount * 6 * sizeof(uint16_t), kLog2MaxMeshesPerCluster) {}

	inline static std::shared_ptr<ChunkMeshPool> Create(const myvk::Ptr<myvk::Device> &device) {
		return std::make_shared<ChunkMeshPool>(device);
	}
};

#endif
