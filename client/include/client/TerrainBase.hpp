#ifndef CUBECRAFT3_CLIENT_TERRAIN_BASE_HPP
#define CUBECRAFT3_CLIENT_TERRAIN_BASE_HPP

#include <atomic>
#include <cinttypes>
#include <functional>
#include <memory>
#include <shared_mutex>
#include <unordered_map>

#include <spdlog/spdlog.h>

class Chunk;
class TerrainBase {
private:
	uint32_t m_seed{};

public:
	inline explicit TerrainBase(uint32_t seed) : m_seed{seed} {}
	virtual ~TerrainBase() = default;
	inline uint32_t GetSeed() const { return m_seed; }

	// chunk: the chunk to be generated; peak: the estimated block peak of the y-axis (used to generate sunlight)
	virtual void Generate(const std::shared_ptr<Chunk> &chunk_ptr, uint32_t *y_peak) = 0;
};

template <typename KEY, typename T, uint32_t SIZE> class TerrainCache {
private:
	std::pair<KEY, std::shared_ptr<T>> m_cache_queue[SIZE];
	uint32_t m_cache_queue_pointer{0};
	std::unordered_map<KEY, std::weak_ptr<T>> m_cache_map;
	std::shared_mutex m_cache_map_mutex;

public:
	template <typename GEN>
	auto Acquire(const KEY &key, GEN generator) -> decltype(generator(KEY{}, (T *){}), std::shared_ptr<const T>{}) {
		{ // try to acquire it first
			std::shared_lock cache_read_lock{m_cache_map_mutex};
			auto it = m_cache_map.find(key);
			if (it != m_cache_map.end()) {
				auto ptr = it->second.lock();
				if (ptr)
					return ptr;
			}
		}

		// if not, generate it
		std::shared_ptr<T> generated = std::make_shared<T>();
		generator(key, generated.get());
		// spdlog::info("cache generated");

		{
			std::scoped_lock cache_write_lock{m_cache_map_mutex};
			auto it = m_cache_map.find(key);
			if (it == m_cache_map.end() || it->second.expired()) {
				// remove deprecated cache
				if (m_cache_queue[m_cache_queue_pointer].second)
					m_cache_map.erase(m_cache_queue[m_cache_queue_pointer].first);

				// update map
				m_cache_map[key] = generated;

				// push new cache to queue
				m_cache_queue[m_cache_queue_pointer++] = {key, generated};
				m_cache_queue_pointer %= SIZE;
			}
		}

		return generated;
	}
};

#endif
