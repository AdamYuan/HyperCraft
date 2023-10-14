#include <client/ChunkTaskPool.hpp>

#include <client/ClientBase.hpp>
#include <client/World.hpp>

#include <algorithm>

namespace hc::client {

struct ChunkTaskSortEntry {
	ChunkPos3 pos;
	uint16_t dist2;
	inline bool operator<(const ChunkTaskSortEntry &r) const { return dist2 < r.dist2; }
};

ChunkTaskPool::RunnerDataVariant ChunkTaskPool::produce_runner_data(std::size_t max_tasks) {
	auto center_pos = m_world.GetCenterChunkPos();
	auto load_radius = m_world.GetLoadChunkRadius(), unload_radius = m_world.GetUnloadChunkRadius();

	std::vector<RunnerDataVariant> new_runner_data_vec;
	{
		std::vector<ChunkTaskSortEntry> sort_entries;

		ChunkTaskPoolLocked locked_pool{this};
		auto &locked_data_map = locked_pool.GetDataMap();
		// Unload and Erase empty data
		for (auto it = locked_data_map.begin();
		     it != locked_data_map.end() && new_runner_data_vec.size() < max_tasks;) {
			bool erase = false;
			const auto &chunk_pos = it->first;
			uint32_t dist2 = ChunkPosDistance2(chunk_pos, center_pos);
			bool unload = dist2 > uint32_t(unload_radius * unload_radius),
			     load = dist2 <= uint32_t(load_radius * load_radius);
			std::apply(
			    [&erase, unload](auto &&...data) {
				    if (unload)
					    ([&] { data.OnUnload(); }(), ...);
				    erase = !(data.IsQueued() || ...);
			    },
			    it->second);

			if (erase)
				it = locked_data_map.erase(it);
			else {
				if (load)
					sort_entries.push_back({chunk_pos, uint16_t(dist2)});
				++it;
			}
		}

		// Sort entries
		std::sort(sort_entries.begin(), sort_entries.end());

		// Push Entries
		for (const auto &e : sort_entries) {
			std::apply(
			    [&e, &new_runner_data_vec, &locked_pool](auto &&...data) {
				    (
				        [&] {
					        auto runner_data_opt = data.Pop(locked_pool, e.pos);
					        if (runner_data_opt.has_value())
						        new_runner_data_vec.emplace_back(std::move(runner_data_opt.value()));
				        }(),
				        ...);
			    },
			    locked_data_map.find(e.pos)->second);
			if (new_runner_data_vec.size() > max_tasks)
				break;
		}
	}
	if (!new_runner_data_vec.empty()) {
		auto ret = std::move(new_runner_data_vec.back());
		new_runner_data_vec.pop_back();

		if (!new_runner_data_vec.empty()) {
			m_all_tasks_done.wait(false, std::memory_order_acquire); // Wait all works to be done

			m_all_tasks_done.store(false, std::memory_order_release);
			m_remaining_tasks.store(new_runner_data_vec.size(), std::memory_order_release);
			m_runner_data_queue.enqueue_bulk(std::make_move_iterator(new_runner_data_vec.begin()),
			                                 new_runner_data_vec.size());
		}
		return ret;
	}
	return std::monostate{};
}

void ChunkTaskPool::Run(ChunkTaskPoolToken *p_token, uint32_t timeout_milliseconds, std::size_t producer_max_tasks) {
	RunnerDataVariant runner_data = std::monostate{};
	if (!m_runner_data_queue.wait_dequeue_timed(p_token->m_consumer_token, runner_data,
	                                            std::chrono::milliseconds(timeout_milliseconds))) {
		std::scoped_lock lock{m_producer_mutex};
		if (!m_runner_data_queue.try_dequeue(p_token->m_consumer_token, runner_data))
			runner_data = produce_runner_data(producer_max_tasks);
	}

	std::visit(
	    [this, p_token](auto &&runner_data) {
		    using T = std::decay_t<decltype(runner_data)>;
		    if constexpr (!std::is_same_v<T, std::monostate>) {
			    std::get<ChunkTaskRunner<T::kType>>(p_token->m_runners).Run(this, std::forward<T>(runner_data));
			    auto remaining_tasks = m_remaining_tasks.fetch_sub(1, std::memory_order_acq_rel) - 1;
			    if (remaining_tasks == 0) {
				    m_all_tasks_done.store(true, std::memory_order_release);
				    m_all_tasks_done.notify_one();
			    }
		    }
	    },
	    runner_data);
}
} // namespace hc::client
