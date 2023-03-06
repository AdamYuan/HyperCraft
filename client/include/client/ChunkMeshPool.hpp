#ifndef CUBECRAFT3_CLIENT_CHUNK_RENDERER_HPP
#define CUBECRAFT3_CLIENT_CHUNK_RENDERER_HPP

#include <cinttypes>

#include "client/mesh/MeshPool.hpp"
#include <client/ChunkMesh.hpp>
#include <common/AABB.hpp>
#include <common/Position.hpp>

class ChunkMeshPool : public ChunkMeshPoolBase {
private:
	inline static constexpr uint32_t kClusterFaceCount = 4 * 1024 * 1024;
	inline static constexpr uint32_t kMaxMeshesPerCluster = 4096u, kMaxClusters = 8u;

public:
	explicit ChunkMeshPool(const myvk::Ptr<myvk::Device> &device)
	    : ChunkMeshPoolBase(device, kClusterFaceCount * 4 * sizeof(ChunkMeshVertex),
	                        kClusterFaceCount * 6 * sizeof(uint16_t), kMaxClusters, kMaxMeshesPerCluster) {}

	inline static std::shared_ptr<ChunkMeshPool> Create(const myvk::Ptr<myvk::Device> &device) {
		return std::make_shared<ChunkMeshPool>(device);
	}
};

#endif
