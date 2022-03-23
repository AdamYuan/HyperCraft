#include <client/ChunkLighter.hpp>

#include <client/ChunkMesher.hpp>

#include <glm/gtc/type_ptr.hpp>

template <typename T, typename = std::enable_if_t<std::is_integral_v<T> && std::is_signed_v<T>>>
static constexpr uint32_t chunk_xyz_extended14_to_index(T x, T y, T z) {
	bool x_inside = 0 <= x && x < kChunkSize, y_inside = 0 <= y && y < kChunkSize, z_inside = 0 <= z && z < kChunkSize;
	uint32_t bits = x_inside | (y_inside << 1u) | (z_inside << 2u);
	if (bits == 7u)
		return ChunkXYZ2Index(x, y, z);
	constexpr uint32_t kOffsets[8] = {
	    kChunkSize * kChunkSize * kChunkSize,
	    kChunkSize * kChunkSize * kChunkSize + 28 * 28 * 28,
	    kChunkSize * kChunkSize * kChunkSize + 28 * 28 * 28 + 28 * kChunkSize * 28,
	    kChunkSize * kChunkSize * kChunkSize + 28 * 28 * 28 + 28 * kChunkSize * 28 * 2,
	    kChunkSize * kChunkSize * kChunkSize + 28 * 28 * 28 + 28 * kChunkSize * 28 * 2 + kChunkSize * kChunkSize * 28,
	    kChunkSize * kChunkSize * kChunkSize + 28 * 28 * 28 + 28 * kChunkSize * 28 * 3 + kChunkSize * kChunkSize * 28,
	    kChunkSize * kChunkSize * kChunkSize + 28 * 28 * 28 + 28 * kChunkSize * 28 * 3 +
	        kChunkSize * kChunkSize * 28 * 2,
	    0};
	constexpr uint32_t kMultipliers[8][3] = {{28, 28, 28},
	                                         {kChunkSize, 28, 28},
	                                         {28, kChunkSize, 28},
	                                         {kChunkSize, kChunkSize, 28},
	                                         {28, 28, kChunkSize},
	                                         {kChunkSize, 28, kChunkSize},
	                                         {28, kChunkSize, kChunkSize},
	                                         {kChunkSize, kChunkSize, kChunkSize}};
	if (!x_inside)
		x = x < 0 ? x + 14 : x - (int32_t)kChunkSize + 14;
	if (!y_inside)
		y = y < 0 ? y + 14 : y - (int32_t)kChunkSize + 14;
	if (!z_inside)
		z = z < 0 ? z + 14 : z - (int32_t)kChunkSize + 14;
	return kOffsets[bits] + kMultipliers[bits][0] * (kMultipliers[bits][2] * y + z) + x;
}

template <typename T, typename = std::enable_if_t<std::is_integral_v<T> && std::is_signed_v<T>>>
static constexpr bool light_interfere(T x, T y, T z, LightLvl lvl) {
	if (lvl <= 1)
		return false;
	uint32_t dist = 0;
	if (x < 0 || x >= kChunkSize)
		dist += x < 0 ? -x : x - (int32_t)kChunkSize;
	if (y < 0 || y >= kChunkSize)
		dist += y < 0 ? -y : y - (int32_t)kChunkSize;
	if (z < 0 || z >= kChunkSize)
		dist += z < 0 ? -z : z - (int32_t)kChunkSize;
	return (uint32_t)lvl > dist;
}

struct LightEntry {
	glm::i16vec3 position;
	LightLvl light_lvl;
};
class LightQueue {
private:
	LightEntry m_entries[(kChunkSize + 28) * (kChunkSize + 28) * (kChunkSize + 28)]{};
	LightEntry *m_back = m_entries, *m_top = m_entries;

public:
	inline void Clear() { m_back = m_top = m_entries; }
	inline bool Empty() const { return m_back == m_top; }
	inline LightEntry Pop() { return *(m_back++); }
	inline void Push(const LightEntry &e) { *(m_top++) = e; }
};

static void initial_sunlight_bfs(Light *light_buffer, LightQueue *queue) {
	while (!queue->Empty()) {
		LightEntry e = queue->Pop();
		for (BlockFace f = 0; f < 6; ++f) {
			LightEntry nei = e;
			BlockFaceProceed(glm::value_ptr(nei.position), f);
			--nei.light_lvl;

			uint32_t idx = chunk_xyz_extended14_to_index(nei.position.x, nei.position.y, nei.position.z);
			if (nei.light_lvl > light_buffer[idx].GetSunlight() &&
			    light_interfere(nei.position.x, nei.position.y, nei.position.z, nei.light_lvl)) {
				light_buffer[idx].SetSunlight(nei.light_lvl);
				queue->Push(nei);
			}
		}
	}
}

void ChunkLighter::Run() {
	if (!lock() || !m_chunk_ptr->IsGenerated())
		return;

	uint64_t version = m_chunk_ptr->FetchLightVersion();
	if (!version)
		return;

	thread_local static Light light_buffer[(kChunkSize + 28) * (kChunkSize + 28) * (kChunkSize + 28)];
	thread_local static LightQueue sunlight_queue, torchlight_queue;

	sunlight_queue.Clear();
	torchlight_queue.Clear();

	if (m_initial_pass) {
		// wait all neighbours to be generated
		for (const auto &i : m_neighbour_chunk_ptr)
			if (!i->IsGenerated()) {
				push_worker(ChunkLighter::Create(m_chunk_ptr, m_initial_pass, std::move(m_mods)));
				return;
			}
		// fetch light
		for (int32_t x = -14; x < (int32_t)kChunkSize + 14; ++x)
			for (int32_t y = -14; y < (int32_t)kChunkSize + 14; ++y)
				for (int32_t z = -14; z < (int32_t)kChunkSize + 14; ++z) {
					Light light = get_light(x, y, z);
					light_buffer[chunk_xyz_extended14_to_index(x, y, z)] = light;
					if (light_interfere(x, y, z, light.GetSunlight()))
						sunlight_queue.Push({{x, y, z}, light.GetSunlight()});
					if (light_interfere(x, y, z, light.GetTorchlight()))
						torchlight_queue.Push({{x, y, z}, light.GetTorchlight()});
				}
		initial_sunlight_bfs(light_buffer, &sunlight_queue);
	}
	m_chunk_ptr->PushLight(version, light_buffer);
	// TODO: Push neighbour lights to be updated
	{}
	/* for (uint32_t i = 0; i < Chunk::kSize * Chunk::kSize * Chunk::kSize; ++i)
		if (m_chunk_ptr->GetBlock(i) != Blocks::kAir) {
			push_worker(ChunkMesher::Create(m_chunk_ptr));
			return;
		} */
}
