#include <common/WorldDatabase.hpp>

#include <spdlog/spdlog.h>

namespace hc {

inline constexpr const char *kConfigDBName = "config", *kBlockDBName = "block", *kSunlightDBName = "sunlight";
inline constexpr char kConfigSeedName[] = "seed";
inline constexpr MDB_val kConfigSeedKey = {.mv_size = sizeof(kConfigSeedName), .mv_data = (void *)kConfigSeedName};

#define MDB_CALL(ERR_RET_VAL, MDB_FUNC, ...) \
	if (const auto err = MDB_FUNC(__VA_ARGS__)) { \
		spdlog::error(#MDB_FUNC " error: {}", err); \
		return ERR_RET_VAL; \
	}

std::unique_ptr<WorldDatabase> WorldDatabase::Create(const char *filename, std::size_t max_size) {
	std::unique_ptr<WorldDatabase> ret = std::make_unique<WorldDatabase>();

	MDB_CALL(nullptr, mdb_env_create, &ret->m_env)
	mdb_env_set_maxdbs(ret->m_env, 8);
	mdb_env_set_mapsize(ret->m_env, max_size);
	MDB_CALL(nullptr, mdb_env_open, ret->m_env, filename, MDB_NOSUBDIR, 0664)

	// create tables
	MDB_txn *txn{};
	mdb_txn_begin(ret->m_env, nullptr, 0, &txn);
	MDB_CALL(nullptr, mdb_dbi_open, txn, kConfigDBName, MDB_CREATE, &ret->m_config_db)
	MDB_CALL(nullptr, mdb_dbi_open, txn, kBlockDBName, MDB_CREATE | MDB_INTEGERKEY, &ret->m_block_db)
	MDB_CALL(nullptr, mdb_dbi_open, txn, kSunlightDBName, MDB_CREATE | MDB_INTEGERKEY, &ret->m_sunlight_db)
	mdb_txn_commit(txn);

	return ret;
}

void WorldDatabase::SetSeed(uint32_t seed) {
	MDB_txn *txn{};
	mdb_txn_begin(m_env, nullptr, 0, &txn);
	MDB_val val{.mv_size = sizeof(uint32_t), .mv_data = &seed};
	mdb_put(txn, m_config_db, (MDB_val *)&kConfigSeedKey, &val, 0);
	mdb_txn_commit(txn);
}

uint32_t WorldDatabase::GetSeed() const {
	MDB_txn *txn{};
	mdb_txn_begin(m_env, nullptr, MDB_RDONLY, &txn);
	MDB_val val{};
	mdb_get(txn, m_config_db, (MDB_val *)&kConfigSeedKey, &val);
	uint32_t ret = *(uint32_t *)val.mv_data;
	mdb_txn_abort(txn);
	return ret;
}

WorldDatabase::~WorldDatabase() { mdb_env_close(m_env); }

DBChunk WorldDatabase::GetChunk(ChunkPos3 chunk_pos) const {
	DBChunk chunk;

	MDB_txn *txn{};
	mdb_txn_begin(m_env, nullptr, MDB_RDONLY, &txn);
	MDB_val key = {.mv_size = sizeof(ChunkPos3), .mv_data = &chunk_pos}, block_val{}, sunlight_val{};
	if (mdb_get(txn, m_block_db, &key, &block_val) != MDB_NOTFOUND)
		chunk.blocks = {(DBChunkBlockEntry *)block_val.mv_data,
		                (DBChunkBlockEntry *)((uint8_t *)block_val.mv_data + block_val.mv_size)};
	if (mdb_get(txn, m_sunlight_db, &key, &sunlight_val) != MDB_NOTFOUND)
		chunk.sunlights = {(DBChunkSunlightEntry *)sunlight_val.mv_data,
		                   (DBChunkSunlightEntry *)((uint8_t *)sunlight_val.mv_data + sunlight_val.mv_size)};
	mdb_txn_abort(txn);

	return chunk;
}

void WorldDatabase::SetBlocks(ChunkPos3 chunk_pos, std::span<const DBChunkBlockEntry> blocks) {
	if (blocks.empty())
		return;

	std::unordered_map<InnerIndex3, block::Block> block_map;
	for (auto b : blocks)
		block_map[b.index] = b.block;

	MDB_txn *txn{};
	mdb_txn_begin(m_env, nullptr, 0, &txn);
	MDB_val key = {.mv_size = sizeof(ChunkPos3), .mv_data = &chunk_pos}, val{};

	std::vector<DBChunkBlockEntry> entries;
	if (mdb_get(txn, m_block_db, &key, &val) != MDB_NOTFOUND) {
		entries = {(DBChunkBlockEntry *)val.mv_data, (DBChunkBlockEntry *)((uint8_t *)val.mv_data + val.mv_size)};
		for (auto &entry : entries) {
			auto it = block_map.find(entry.index);
			if (it != block_map.end()) {
				entry.block = it->second;
				block_map.erase(it);
			}
		}
	}
	for (const auto &b : block_map)
		entries.push_back({b.first, b.second});

	val = {.mv_size = entries.size() * sizeof(DBChunkBlockEntry), .mv_data = entries.data()};
	mdb_put(txn, m_block_db, &key, &val, 0);
	mdb_txn_commit(txn);
}

void WorldDatabase::SetSunlights(ChunkPos3 chunk_pos, std::span<const DBChunkSunlightEntry> sunlights) {
	if (sunlights.empty())
		return;

	std::unordered_map<InnerIndex2, InnerPos1> sunlight_map;
	for (auto b : sunlights)
		sunlight_map[b.GetIndex()] = b.GetSunlight();

	MDB_txn *txn{};
	mdb_txn_begin(m_env, nullptr, 0, &txn);
	MDB_val key = {.mv_size = sizeof(ChunkPos3), .mv_data = &chunk_pos}, val{};

	std::vector<DBChunkSunlightEntry> entries;
	if (mdb_get(txn, m_sunlight_db, &key, &val) != MDB_NOTFOUND) {
		entries = {(DBChunkSunlightEntry *)val.mv_data, (DBChunkSunlightEntry *)((uint8_t *)val.mv_data + val.mv_size)};
		for (auto &entry : entries) {
			auto it = sunlight_map.find(entry.GetIndex());
			if (it != sunlight_map.end()) {
				entry = {it->first, it->second};
				sunlight_map.erase(it);
			}
		}
	}
	for (const auto &b : sunlight_map)
		entries.push_back({b.first, b.second});

	val = {.mv_size = entries.size() * sizeof(DBChunkSunlightEntry), .mv_data = entries.data()};
	mdb_put(txn, m_sunlight_db, &key, &val, 0);
	mdb_txn_commit(txn);
}

} // namespace hc