#ifndef DATABASE_HPP
#define DATABASE_HPP

#include <memory>
#include <sqlite3.h>

class LevelDB {
private:
	sqlite3 *m_db{nullptr};

public:
	static std::shared_ptr<LevelDB> Create(const char *filename);

	void SetSeed(uint32_t seed);
	uint32_t GetSeed();

	~LevelDB();
};

#endif
