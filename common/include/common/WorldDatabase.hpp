#ifndef CUBECRAFT3_COMMON_DATABASE_HPP
#define CUBECRAFT3_COMMON_DATABASE_HPP

#include <memory>
#include <sqlite3.h>

class WorldDatabase {
private:
	sqlite3 *m_db{nullptr};

public:
	static std::shared_ptr<WorldDatabase> Create(const char *filename);

	void SetSeed(uint32_t seed);
	uint32_t GetSeed();

	~WorldDatabase();
};

#endif
