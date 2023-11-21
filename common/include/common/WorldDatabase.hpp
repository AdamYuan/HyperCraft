#ifndef HYPERCRAFT_COMMON_DATABASE_HPP
#define HYPERCRAFT_COMMON_DATABASE_HPP

#include <common/Data.hpp>
#include <lmdb.h>
#include <memory>
#include <span>
#include <vector>

namespace hc {

class WorldDatabase {
private:
	MDB_env *m_env{nullptr};
	MDB_dbi m_config_db{}, m_block_db{}, m_sunlight_db{};

public:
	static std::unique_ptr<WorldDatabase> Create(const char *filename, std::size_t max_size = 1024 * 1024 * 1024);

	void SetSeed(uint32_t seed);
	[[nodiscard]] uint32_t GetSeed() const;

	static_assert(sizeof(ChunkPos1) == sizeof(unsigned short) && sizeof(ChunkPos3) == 3 * sizeof(unsigned short));

	[[nodiscard]] std::vector<PackedChunkEntry> GetChunks(std::span<const ChunkPos3> chunk_pos_s) const;
	void SetBlocks(ChunkPos3 chunk_pos, std::span<const ChunkBlockEntry> blocks);
	void SetSunlights(ChunkPos3 chunk_pos, std::span<const ChunkSunlightEntry> sunlights);

	~WorldDatabase();
};

} // namespace hc

#endif
