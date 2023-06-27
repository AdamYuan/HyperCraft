#pragma once

#include <atomic>
#include <mutex>

namespace hc::client {

class ChunkSignal {
private:
	std::atomic_bool m_pending{};
	std::atomic_uint64_t m_version{};
	std::mutex m_done_mutex;

public:
	inline bool IsPending() { return m_pending.load(std::memory_order_acquire); }
	inline void Pend() { m_pending.store(true, std::memory_order_release); }
	inline void Cancel() { m_pending.store(false, std::memory_order_release); }
	inline uint64_t FetchVersion() {
		m_pending.store(false, std::memory_order_release);
		return m_version.fetch_add(1, std::memory_order_acq_rel) + 1;
	}
	template <typename DoneFunc> inline bool Done(uint64_t version, DoneFunc &&done_func) {
		std::scoped_lock lock{m_done_mutex};
		if (version == m_version.load(std::memory_order_acquire)) {
			done_func();
			return true;
		}
		return false;
	}
};

} // namespace hc::client
