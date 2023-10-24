#pragma once

#include <block/BlockFace.hpp>
#include <block/Light.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <queue>

#include "BlockAlgo.hpp"

namespace hc::client {

template <typename Config, uint32_t Border = 0> class BlockLightAlgo {
private:
	using T = typename Config::Type;

public:
	struct Entry {
		glm::vec<3, T> pos;
		block::LightLvl lvl;
	};
	using Queue = std::queue<Entry>;

private:
	Queue m_queue;

	static inline constexpr bool in_bound_bordered(T x, T y, T z) {
		constexpr BlockAlgoBound<T> kBound = Config::kBound;
		return kBound.min_x - (T)Border <= x && x < kBound.max_x + (T)Border && kBound.min_y - (T)Border <= y &&
		       y < kBound.max_y + (T)Border && kBound.min_z - (T)Border <= z && z < kBound.max_z + (T)Border;
	}

public:
	static inline constexpr bool IsBorderLightInterfere(T x, T y, T z, block::LightLvl lvl) {
		constexpr BlockAlgoBound<T> kBound = Config::kBound;
		uint32_t dist = 0;
		if (x < kBound.min_x || x >= kBound.max_x)
			dist += x < kBound.min_x ? -(uint32_t)x + (uint32_t)kBound.min_x : (uint32_t)x - (uint32_t)kBound.max_x;
		if (y < kBound.min_y || y >= kBound.max_y)
			dist += y < kBound.min_y ? -(uint32_t)y + (uint32_t)kBound.min_y : (uint32_t)y - (uint32_t)kBound.max_y;
		if (z < kBound.min_z || z >= kBound.max_z)
			dist += z < kBound.min_z ? -(uint32_t)z + (uint32_t)kBound.min_z : (uint32_t)z - (uint32_t)kBound.max_z;
		return (uint32_t)lvl > dist;
	}

	template <typename GetBlockFunc, typename GetLightFunc, typename SetLightFunc>
	inline void PropagateLight(Queue &&entries, GetBlockFunc get_block_func, GetLightFunc get_light_func,
	                           SetLightFunc set_light_func) {

		m_queue = std::move(entries);
		while (!m_queue.empty()) {
			Entry e = m_queue.front();
			m_queue.pop();
			if (e.lvl <= 1u)
				continue;
			--e.lvl;
			for (block::BlockFace f = 0; f < 6; ++f) {
				Entry nei = e;
				block::BlockFaceProceed(glm::value_ptr(nei.pos), f);

				if (!in_bound_bordered(nei.pos.x, nei.pos.y, nei.pos.z))
					continue;

				if constexpr (Border > 0)
					if (!IsBorderLightInterfere(nei.pos.x, nei.pos.y, nei.pos.z, nei.lvl))
						continue;

				if (nei.lvl > get_light_func(nei.pos.x, nei.pos.y, nei.pos.z) &&
				    get_block_func(nei.pos.x, nei.pos.y, nei.pos.z).GetIndirectLightPass()) {
					set_light_func(nei.pos.x, nei.pos.y, nei.pos.z, nei.lvl);
					if (nei.lvl > 1)
						m_queue.push(nei);
				}
			}
		}
	}
};

} // namespace hc::client