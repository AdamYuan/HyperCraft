#ifndef DATABASE_HPP
#define DATABASE_HPP

#include <memory>
#include <sqlite3.h>

class Database {
private:
	sqlite3 *m_db{nullptr};

public:
	static std::shared_ptr<Database> Create(const char *filename);
	~Database();
};

#endif
