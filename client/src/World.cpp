#include <client/World.hpp>

#include <glm/gtx/string_cast.hpp>
#include <spdlog/spdlog.h>

#include <client/Config.hpp>

namespace hc::client {

World::~World() {
	/* auto locked_chunks = m_chunks.lock_table();
	for (const auto &i : locked_chunks)
		i.second->SetMeshFinalize(); */
}

} // namespace hc::client
