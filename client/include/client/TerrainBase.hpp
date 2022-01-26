#ifndef CUBECRAFT3_CLIENT_TERRAIN_BASE_HPP
#define CUBECRAFT3_CLIENT_TERRAIN_BASE_HPP

#include <memory>

class Chunk;
class ClientBase;

class TerrainBase {
private:
	std::weak_ptr<ClientBase> m_client_weak_ptr;

public:
	virtual void Generate(const std::shared_ptr<Chunk> &chunk) = 0;
	virtual ~TerrainBase() = default;

	inline const std::weak_ptr<ClientBase> &GetClientWeakPtr() const { return m_client_weak_ptr; }
	inline std::shared_ptr<ClientBase> LockClient() const { return m_client_weak_ptr.lock(); }
};

#endif
