#ifndef CUBECRAFT3_CLIENT_CLIENT_BASE_HPP
#define CUBECRAFT3_CLIENT_CLIENT_BASE_HPP

#include <client/World.hpp>

class ClientBase : public std::enable_shared_from_this<ClientBase> {
private:
	std::shared_ptr<World> m_world_ptr;

public:
	inline explicit ClientBase(const std::shared_ptr<World> &world_ptr) {
		world_ptr->m_client_weak_ptr = weak_from_this();
		m_world_ptr = world_ptr;
	}
	inline const std::shared_ptr<World> &GetWorldPtr() const { return m_world_ptr; }
	virtual void LoadChunk(const ChunkPos3 &chk_pos) = 0;
	virtual void SetBlock(const ChunkPos3 &chk_pos, uint32_t index, Block block) = 0;
};

#endif
