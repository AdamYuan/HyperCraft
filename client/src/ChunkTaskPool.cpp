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

void ChunkTaskPool::produce_runner_data(ChunkTaskPoolToken *p_token, std::size_t max_tasks) {
	auto center_pos = m_world.GetCenterChunkPos();
	auto load_radius = m_world.GetLoadChunkRadius(), unload_radius = m_world.GetUnloadChunkRadius();

	std::vector<RunnerDataVariant> hp_runner_data_vec, lp_runner_data_vec;
	{
		std::vector<ChunkTaskSortEntry> sort_entries;

		ChunkTaskPoolLocked locked_pool{this};
		auto &locked_data_map = locked_pool.GetDataMap();
		// Unload and Erase empty data
		for (auto it = locked_data_map.begin(); it != locked_data_map.end();) {
			bool erase = false;
			const auto &chunk_pos = it->first;
			uint32_t dist2 = ChunkPosDistance2(chunk_pos, center_pos);
			bool unload = dist2 > uint32_t(unload_radius * unload_radius),
			     load = dist2 <= uint32_t(load_radius * load_radius);
			std::apply(
			    [&erase, unload](auto &&...data) {
				    if (unload)
					    ([&] { data.OnUnload(); }(), ...);
				    erase = !(data.NotIdle() || ...);
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
			    [&e, &hp_runner_data_vec, &lp_runner_data_vec, &locked_pool](auto &&...data) {
				    (
				        [&] {
					        if (data.m_running)
						        return;
					        auto runner_data_opt = data.Pop(locked_pool, e.pos);
					        if (runner_data_opt.has_value()) {
						        (data.m_priority == ChunkTaskPriority::kHigh ? hp_runner_data_vec : lp_runner_data_vec)
						            .emplace_back(std::move(runner_data_opt.value()));

						        data.m_running = true;
						        data.m_priority = ChunkTaskPriority::kLow;
					        }
				        }(),
				        ...);
			    },
			    locked_data_map.find(e.pos)->second);
			if (lp_runner_data_vec.size() > max_tasks)
				break;
		}
	}
	if (!lp_runner_data_vec.empty())
		m_low_priority_runner_data_queue.enqueue_bulk(p_token->m_low_priority_producer_token,
		                                              std::make_move_iterator(lp_runner_data_vec.begin()),
		                                              lp_runner_data_vec.size());
	if (!hp_runner_data_vec.empty())
		m_high_priority_runner_data_queue.enqueue_bulk(p_token->m_high_priority_producer_token,
		                                               std::make_move_iterator(hp_runner_data_vec.begin()),
		                                               hp_runner_data_vec.size());
}

void ChunkTaskPool::Run(ChunkTaskPoolToken *p_token, std::size_t producer_max_tasks) {
	RunnerDataVariant runner_data = std::monostate{};
	if (m_high_priority_producer_flag.exchange(false, std::memory_order_acq_rel)) {
		std::scoped_lock lock{m_producer_mutex};
		if (!m_high_priority_runner_data_queue.try_dequeue(p_token->m_high_priority_consumer_token, runner_data)) {
			produce_runner_data(p_token, producer_max_tasks);
			return;
		}
	} else if (!m_high_priority_runner_data_queue.try_dequeue(p_token->m_high_priority_consumer_token, runner_data) &&
	           !m_low_priority_runner_data_queue.try_dequeue(p_token->m_low_priority_consumer_token, runner_data)) {
		std::scoped_lock lock{m_producer_mutex};
		if (!m_high_priority_runner_data_queue.try_dequeue(p_token->m_high_priority_consumer_token, runner_data) &&
		    !m_low_priority_runner_data_queue.try_dequeue(p_token->m_low_priority_consumer_token, runner_data)) {
			produce_runner_data(p_token, producer_max_tasks);
			return;
		}
	}

	std::visit(
	    [this, p_token](auto &&runner_data) {
		    using T = std::decay_t<decltype(runner_data)>;
		    if constexpr (!std::is_same_v<T, std::monostate>) {
			    std::get<ChunkTaskRunner<T::kType>>(p_token->m_runners).Run(this, std::forward<T>(runner_data));
			    m_data_map.find_fn(runner_data.GetChunkPos(), [](auto &data) {
				    std::get<static_cast<std::size_t>(T::kType)>(data).m_running = false;
			    });
		    }
	    },
	    runner_data);
}
} // namespace hc::client
