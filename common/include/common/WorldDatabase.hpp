#ifndef HYPERCRAFT_COMMON_DATABASE_HPP
#define HYPERCRAFT_COMMON_DATABASE_HPP

#include <block/Block.hpp>
#include <common/Position.hpp>
#include <lmdb.h>
#include <memory>
#include <span>
#include <vector>

namespace hc {

struct DBChunkBlockEntry {
	InnerIndex3 index{};
	block::Block block;
};
static_assert(sizeof(DBChunkBlockEntry) == 4);
struct DBChunkSunlightEntry {
	InnerIndex2 index : 10;
	InnerPos1 sunlight : 6;
};
static_assert((kChunkSize + 1) <= (1 << 6) && (kChunkSize * kChunkSize <= (1 << 10)));
static_assert(sizeof(DBChunkSunlightEntry) == 2);

struct DBChunk {
	std::vector<DBChunkBlockEntry> blocks;
	std::vector<DBChunkSunlightEntry> sunlights;
};

class WorldDatabase {
private:
	MDB_env *m_env{nullptr};
	MDB_dbi m_config_db{}, m_block_db{}, m_sunlight_db{};

public:
	static std::unique_ptr<WorldDatabase> Create(const char *filename, std::size_t max_size = 1024 * 1024 * 1024);

	void SetSeed(uint32_t seed);
	[[nodiscard]] uint32_t GetSeed() const;

	static_assert(sizeof(ChunkPos1) == sizeof(unsigned short) && sizeof(ChunkPos3) == 3 * sizeof(unsigned short));

	[[nodiscard]] DBChunk GetChunk(ChunkPos3 chunk_pos) const;
	void SetBlocks(ChunkPos3 chunk_pos, std::span<const DBChunkBlockEntry> blocks);
	void SetSunlights(ChunkPos3 chunk_pos, std::span<const DBChunkSunlightEntry> sunlights);

	~WorldDatabase();
};

} // namespace hc

#endif
