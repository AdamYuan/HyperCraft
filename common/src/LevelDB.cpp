#include <common/LevelDB.hpp>

std::shared_ptr<LevelDB> LevelDB::Create(const char *filename) {
	std::shared_ptr<LevelDB> ret = std::make_shared<LevelDB>();
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

void LevelDB::SetSeed(uint32_t seed) {

}

uint32_t LevelDB::GetSeed() {
	return 0;
}

LevelDB::~LevelDB() {
	if (m_db)
		sqlite3_close_v2(m_db);
}
