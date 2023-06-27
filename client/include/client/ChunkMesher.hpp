#ifndef HYPERCRAFT_CLIENT_CHUNK_MESHER_HPP
#define HYPERCRAFT_CLIENT_CHUNK_MESHER_HPP

#include <AABB.hpp>
#include <client/ChunkMesh.hpp>
#include <client/ChunkWorkerBase.hpp>
#include <queue>

namespace hc::client {

class ChunkMesher final : public ChunkWorkerS26Base {
public:
	static inline std::unique_ptr<ChunkMesher> TryCreate(const std::shared_ptr<Chunk> &chunk_ptr) {
		if (chunk_ptr->GetMeshSync().IsPending())
			return nullptr;
		chunk_ptr->GetMeshSync().Pend();
		return std::make_unique<ChunkMesher>(chunk_ptr, false);
	}
	static inline std::unique_ptr<ChunkMesher> TryCreateWithInitialLight(const std::shared_ptr<Chunk> &chunk_ptr) {
		if (chunk_ptr->GetMeshSync().IsPending() && chunk_ptr->GetLightSync().IsPending())
			return nullptr;
		chunk_ptr->GetMeshSync().Pend();
		chunk_ptr->GetLightSync().Pend();
		return std::make_unique<ChunkMesher>(chunk_ptr, true);
	}

	explicit ChunkMesher(const std::shared_ptr<Chunk> &chunk_ptr, bool init_light = false)
	    : ChunkWorkerS26Base(chunk_ptr), m_init_light{init_light} {}
	~ChunkMesher() override = default;

	void Run() override;

private:
	inline bool lock() final {
		if (!ChunkWorkerS26Base::lock()) {
			if (m_chunk_ptr) {
				m_chunk_ptr->GetMeshSync().Cancel();
				if (m_init_light)
					m_chunk_ptr->GetLightSync().Cancel();
			}
			return false;
		}
		if (!m_chunk_ptr->IsGenerated()) {
			m_chunk_ptr->GetMeshSync().Cancel();
			if (m_init_light)
				m_chunk_ptr->GetLightSync().Cancel();
			return false;
		}
		m_version = m_chunk_ptr->GetMeshSync().FetchVersion();
		if (m_init_light)
			m_light_version = m_chunk_ptr->GetLightSync().FetchVersion();
		return true;
	}
	bool m_init_light{};
	uint64_t m_version{}, m_light_version{};
	inline static thread_local block::Light m_light_buffer[(kChunkSize + 30) * (kChunkSize + 30) * (kChunkSize + 30)]{};
};

} // namespace hc::client

#endif
