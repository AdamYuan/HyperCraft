#include <common/WorldDatabase.hpp>

#include <spdlog/spdlog.h>

namespace hc {

std::unique_ptr<WorldDatabase> WorldDatabase::Create(const char *filename) {
	std::unique_ptr<WorldDatabase> ret = std::make_unique<WorldDatabase>();
	if (sqlite3_open_v2(filename, &ret->m_db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, nullptr) != SQLITE_OK)
		return nullptr;

	// Create tables
	constexpr const char *kCreateQueriesStr = "create table if not exists Blocks ("
	                                          " x int not null,"
	                                          "	y int not null,"
	                                          "	z int not null,"
	                                          "	i int not null,"
	                                          "	b int not null,"
	                                          "	primary key(x, y, z, i)"
	                                          ");"
	                                          "create table if not exists Properties ("
	                                          " key text not null, value"
	                                          ");";
	if (sqlite3_exec(ret->m_db, kCreateQueriesStr, nullptr, nullptr, nullptr) != SQLITE_OK)
		return nullptr;

	// Create statements
	constexpr const char *kGetPropertyStr = "select value from Properties where key = ?";
	if (sqlite3_prepare_v2(ret->m_db, kGetPropertyStr, -1, &ret->m_get_property_stmt, nullptr) != SQLITE_OK)
		return nullptr;
	constexpr const char *kSetPropertyStr = "insert or replace into Properties (key, value) values (?, ?);";
	if (sqlite3_prepare_v2(ret->m_db, kSetPropertyStr, -1, &ret->m_set_property_stmt, nullptr) != SQLITE_OK)
		return nullptr;

	return ret;
}

void WorldDatabase::SetSeed(uint32_t seed) {}

uint32_t WorldDatabase::GetSeed() { return 0; }

WorldDatabase::~WorldDatabase() {
	if (m_get_property_stmt)
		sqlite3_finalize(m_get_property_stmt);
	if (m_set_property_stmt)
		sqlite3_finalize(m_set_property_stmt);
	if (m_db)
		sqlite3_close_v2(m_db);
}

} // namespace hc