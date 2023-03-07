#ifndef CUBECRAFT3_CLIENT_WORLD_RENDERER_HPP
#define CUBECRAFT3_CLIENT_WORLD_RENDERER_HPP

#include <client/Camera.hpp>
#include <client/ChunkMesh.hpp>
#include <client/Config.hpp>
#include <client/GlobalTexture.hpp>
#include <client/World.hpp>

#include <myvk/Buffer.hpp>
#include <myvk/CommandBuffer.hpp>
#include <myvk/DescriptorSet.hpp>
#include <myvk/GraphicsPipeline.hpp>

#include <queue>
#include <unordered_map>
#include <vector>

class WorldRenderer : public std::enable_shared_from_this<WorldRenderer> {
private:
	std::shared_ptr<ChunkMeshPool> m_chunk_mesh_pool;

	std::shared_ptr<World> m_world_ptr;

public:
	inline static std::shared_ptr<WorldRenderer> Create(const myvk::Ptr<myvk::Device> &device,
	                                                    const std::shared_ptr<World> &world_ptr) {
		std::shared_ptr<WorldRenderer> ret = std::make_shared<WorldRenderer>();
		world_ptr->m_world_renderer_weak_ptr = ret->weak_from_this();
		ret->m_world_ptr = world_ptr;
		ret->m_chunk_mesh_pool = ChunkMeshPool::Create(device);
		return ret;
	}

	inline const std::shared_ptr<World> &GetWorldPtr() const { return m_world_ptr; }

	inline const auto &GetChunkMeshPool() const { return m_chunk_mesh_pool; }
};

#endif
