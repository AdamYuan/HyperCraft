#ifndef CUBECRAFT3_CLIENT_LOCAL_CLIENT_HPP
#define CUBECRAFT3_CLIENT_LOCAL_CLIENT_HPP

#include <client/ClientBase.hpp>
#include <common/LevelDB.hpp>

class LocalClient : public ClientBase {
private:
	std::shared_ptr<LevelDB> m_level_db_ptr;

public:
	inline explicit LocalClient(const std::shared_ptr<World> &world_ptr, const std::shared_ptr<LevelDB> &level_db_ptr)
	    : ClientBase{world_ptr}, m_level_db_ptr{level_db_ptr} {}
	static std::shared_ptr<LocalClient> Create(const std::shared_ptr<World> &world_ptr,
	                                           const std::shared_ptr<LevelDB> &level_db_ptr);

	inline const std::shared_ptr<LevelDB> &GetLevelDBPtr() const { return m_level_db_ptr; }

	void LoadChunk(const ChunkPos3 &chk_pos) override {}
	void SetBlock(const ChunkPos3 &chk_pos, uint32_t index, Block block) override {}
};

#endif
