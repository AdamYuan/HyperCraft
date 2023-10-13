#include <client/ChunkTaskPool.hpp>

#include <client/ClientBase.hpp>
#include <client/World.hpp>

namespace hc::client {

#include "WorldLoadingList.inl"
ChunkTaskPool::RunnerDataVariant ChunkTaskPool::produce_runner_data(std::size_t max_tasks) {

	auto center_pos = m_world.GetCenterChunkPos();
	auto load_radius = m_world.GetLoadChunkRadius();
	std::vector<RunnerDataVariant> new_runner_data_vec;
	for (const ChunkPos3 *i = kWorldLoadingList;
	     i != kWorldLoadingRadiusEnd[load_radius] && new_runner_data_vec.size() < max_tasks; ++i) {
		ChunkPos3 pos = center_pos + *i;
		m_data_map.find_fn(pos, [this, &new_runner_data_vec, &pos](auto &data) {
			std::apply(
			    [&](auto &&...data) {
				    (
				        [&] {
					        auto runner_data_opt = data.Pop(*this, pos);
					        if (runner_data_opt.has_value())
						        new_runner_data_vec.emplace_back(std::move(runner_data_opt.value()));
				        }(),
				        ...);
			    },
			    data);
		});
	}
	{ // Erase Empty Cells
		auto locked_table = m_data_map.lock_table();
		// TODO: Sort with distance to center
		for (auto it = locked_table.begin(); it != locked_table.end();) {
			bool erase = false;
			std::apply([&erase](auto &&...data) { erase = !(data.IsQueued() || ...); }, it->second);

			if (erase)
				it = locked_table.erase(it);
			else
				++it;
		}
	}
	if (!new_runner_data_vec.empty()) {
		auto ret = std::move(new_runner_data_vec.back());
		new_runner_data_vec.pop_back();

		if (!new_runner_data_vec.empty()) {
			m_all_tasks_done.wait(false); // Wait all works to be done

			m_all_tasks_done.store(false);
			m_remaining_tasks.store(new_runner_data_vec.size());
			m_runner_data_queue.enqueue_bulk(std::make_move_iterator(new_runner_data_vec.begin()),
			                                 new_runner_data_vec.size());
		}
		return ret;
	}
	return std::monostate{};
}

void ChunkTaskPool::Run(hc::client::ChunkTaskPoolToken *p_token, uint32_t timeout_milliseconds,
                        std::size_t producer_max_tasks) {
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
			    auto remaining_tasks = m_remaining_tasks.fetch_sub(1) - 1;
			    if (remaining_tasks == 0) {
				    m_all_tasks_done.store(true);
				    m_all_tasks_done.notify_one();
			    }
		    }
	    },
	    runner_data);
}
} // namespace hc::client
