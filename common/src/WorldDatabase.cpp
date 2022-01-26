#include <common/WorldDatabase.hpp>

std::shared_ptr<WorldDatabase> WorldDatabase::Create(const char *filename) {
	std::shared_ptr<WorldDatabase> ret = std::make_shared<WorldDatabase>();
	if (sqlite3_open_v2(filename, &ret->m_db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, nullptr) != SQLITE_OK)
		return nullptr;

	constexpr const char *kCreateQueriesStr = "create table if not exists chunks ("
	                                          " x int not null,"
	                                          "	y int not null,"
	                                          "	i int not null,"
	                                          "	b int not null,"
	                                          "	primary key(x, y, i)"
	                                          ");"
	                                          "create table if not exists property ("
	                                          " key text not null, value"
	                                          ");";
	if (sqlite3_exec(ret->m_db, kCreateQueriesStr, nullptr, nullptr, nullptr) != SQLITE_OK)
		return nullptr;

	return ret;
}

void WorldDatabase::SetSeed(uint32_t seed) {

}

uint32_t WorldDatabase::GetSeed() {
	return 0;
}

WorldDatabase::~WorldDatabase() {
	if (m_db)
		sqlite3_close_v2(m_db);
}
