#include <client/Chunk.hpp>

namespace hc::client {

template <>
class ChunkTaskData<ChunkTaskType::kFloodSunlight> final : public ChunkTaskDataBase<ChunkTaskType::kFloodSunlight> {
private:
	std::vector<InnerPos2> m_xz_updates;

public:
	inline static constexpr ChunkTaskType kType = ChunkTaskType::kFloodSunlight;

	inline void Push(InnerPos2 xz_update) { m_xz_updates.push_back(xz_update); }
	inline void Push(std::span<InnerPos2> xz_updates) {
		m_xz_updates.insert(m_xz_updates.end(), xz_updates.begin(), xz_updates.end());
	}
	inline bool IsQueued() const { return !m_xz_updates.empty(); }
	std::optional<ChunkTaskRunnerData<ChunkTaskType::kFloodSunlight>> Pop(const ChunkTaskPoolLocked &task_pool,
	                                                                      const ChunkPos3 &chunk_pos);

	inline void OnUnload() {}
};

template <> class ChunkTaskRunnerData<ChunkTaskType::kFloodSunlight> {
private:
	std::shared_ptr<Chunk> m_chunk_ptr, m_up_chunk_ptr;
	std::vector<InnerPos2> m_xz_updates;

public:
	inline static constexpr ChunkTaskType kType = ChunkTaskType::kFloodSunlight;

	inline ChunkTaskRunnerData(std::shared_ptr<Chunk> chunk_ptr, std::shared_ptr<Chunk> up_chunk_ptr,
	                           std::vector<InnerPos2> &&xz_updates)
	    : m_chunk_ptr{std::move(chunk_ptr)}, m_up_chunk_ptr{std::move(up_chunk_ptr)},
	      m_xz_updates{std::move(xz_updates)} {}
	inline const ChunkPos3 &GetChunkPos() const { return m_chunk_ptr->GetPosition(); }
	inline const std::shared_ptr<Chunk> &GetChunkPtr() const { return m_chunk_ptr; }
	inline const std::shared_ptr<Chunk> &GetUpChunkPtr() const { return m_up_chunk_ptr; }
	inline const auto &GetXZUpdates() const { return m_xz_updates; }
};

template <> class ChunkTaskRunner<ChunkTaskType::kFloodSunlight> {
private:
public:
	inline static constexpr ChunkTaskType kType = ChunkTaskType::kFloodSunlight;

	void Run(ChunkTaskPool *p_task_pool, ChunkTaskRunnerData<ChunkTaskType::kFloodSunlight> &&data);
};

} // namespace hc::client
