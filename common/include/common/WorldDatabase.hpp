#ifndef HYPERCRAFT_COMMON_DATABASE_HPP
#define HYPERCRAFT_COMMON_DATABASE_HPP

#include <memory>
#include <sqlite3.h>

namespace hc {

class WorldDatabase {
private:
	sqlite3 *m_db{nullptr};
	sqlite3_stmt *m_set_property_stmt{}, *m_get_property_stmt{};

public:
	static std::unique_ptr<WorldDatabase> Create(const char *filename); // TODO: Make it unique_ptr
	// TODO: Implement Clone() for multithreading use case

	void SetSeed(uint32_t seed);
	uint32_t GetSeed();

	~WorldDatabase();
};

} // namespace hc

#endif
