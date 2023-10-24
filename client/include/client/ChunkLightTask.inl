#include <client/Chunk.hpp>

namespace hc::client {

enum class ChunkLightTaskAction { kPropagate, kRemove };

class ChunkLightTaskEntry {
private:
	InnerPos3 m_pos;
	uint8_t m_data;

public:
	inline static ChunkLightTaskEntry MakePropagation(InnerPos3 pos, block::LightType light_type) {
		ChunkLightTaskEntry ret = {};
		ret.m_pos = pos;
		ret.m_data = (static_cast<uint32_t>(light_type) << 7u) | 0x10u;
		return ret;
	}
	inline static ChunkLightTaskEntry MakePropagation(InnerPos3 pos, block::LightType light_type,
	                                                  block::LightLvl light_lvl) {
		ChunkLightTaskEntry ret = {};
		ret.m_pos = pos;
		ret.m_data = (static_cast<uint32_t>(light_type) << 7u) | (light_lvl & 0xfu);
		return ret;
	}
	inline static ChunkLightTaskEntry MakeRemoval(InnerPos3 pos, block::LightType light_type,
	                                              block::LightLvl light_lvl) {
		ChunkLightTaskEntry ret = {};
		ret.m_pos = pos;
		ret.m_data = (static_cast<uint32_t>(light_type) << 7u) | 0x40u | (light_lvl & 0xfu);
		return ret;
	}
	inline const auto &GetPosition() const { return m_pos; }
	inline block::LightType GeLightType() const { return static_cast<block::LightType>(m_data >> 7u); }
	inline ChunkLightTaskAction GetAction() const { return static_cast<ChunkLightTaskAction>((m_data >> 6u) & 1u); }
	inline std::optional<block::LightLvl> GetLightLevel() const {
		return (m_data & 0x10u) ? std::optional<block::LightLvl>{} : block::LightLvl(m_data & 0xfu);
	}
};

template <> class ChunkTaskRunnerData<ChunkTaskType::kLight> {
private:
	std::array<std::shared_ptr<Chunk>, 27> m_chunk_ptr_array;
	std::vector<ChunkLightTaskEntry> m_light_updates;

public:
	inline static constexpr ChunkTaskType kType = ChunkTaskType::kLight;

	inline ChunkTaskRunnerData(std::array<std::shared_ptr<Chunk>, 27> &&chunk_ptr_array,
	                           std::vector<ChunkLightTaskEntry> &&light_updates)
	    : m_chunk_ptr_array{std::move(chunk_ptr_array)}, m_light_updates{std::move(light_updates)} {}
	inline const ChunkPos3 &GetChunkPos() const { return m_chunk_ptr_array[26]->GetPosition(); }
	inline const std::shared_ptr<Chunk> &GetChunkPtr() const { return m_chunk_ptr_array[26]; }
	inline const auto &GetChunkPtrArray() const { return m_chunk_ptr_array; }
	inline const auto &GetLightUpdates() const { return m_light_updates; }
};

template <> class ChunkTaskData<ChunkTaskType::kLight> final : public ChunkTaskDataBase<ChunkTaskType::kLight> {
private:
	std::vector<ChunkLightTaskEntry> m_light_updates;

public:
	inline static constexpr ChunkTaskType kType = ChunkTaskType::kLight;

	inline void Push(const ChunkLightTaskEntry &entry) { m_light_updates.push_back(entry); }
	inline void Push(std::vector<ChunkLightTaskEntry> &&entries) {
		m_light_updates.insert(m_light_updates.end(), entries.begin(), entries.end());
	}
	inline bool IsQueued() const { return !m_light_updates.empty(); }
	std::optional<ChunkTaskRunnerData<ChunkTaskType::kLight>> Pop(const ChunkTaskPoolLocked &task_pool,
	                                                              const ChunkPos3 &chunk_pos);
	inline void OnUnload() {}
};

template <> class ChunkTaskRunner<ChunkTaskType::kLight> {
private:
public:
	inline static constexpr ChunkTaskType kType = ChunkTaskType::kLight;

	void Run(ChunkTaskPool *p_task_pool, ChunkTaskRunnerData<ChunkTaskType::kLight> &&data);
};

} // namespace hc::client
