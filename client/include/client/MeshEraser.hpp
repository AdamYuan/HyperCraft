#ifndef CUBECRAFT3_CLIENT_MESH_ERASER_HPP
#define CUBECRAFT3_CLIENT_MESH_ERASER_HPP

#include <client/MeshHandle.hpp>
#include <client/WorkerBase.hpp>
#include <set>
#include <utility>

#include <spdlog/spdlog.h>

template <typename Vertex, typename Index, typename Info, uint32_t kPassCount> class MeshEraser : public WorkerBase {
private:
	std::vector<std::unique_ptr<MeshHandle<Vertex, Index, Info, kPassCount>>> m_mesh_handle_vector;

public:
	inline explicit MeshEraser(std::unique_ptr<MeshHandle<Vertex, Index, Info, kPassCount>> &&mesh_handle_ptr)
	    : m_mesh_handle_vector({std::move(mesh_handle_ptr)}) {}
	inline explicit MeshEraser(std::vector<std::unique_ptr<MeshHandle<Vertex, Index, Info, kPassCount>>> &&mesh_handle_vector)
	    : m_mesh_handle_vector{std::move(mesh_handle_vector)} {}

	static inline std::unique_ptr<MeshEraser>
	Create(std::unique_ptr<MeshHandle<Vertex, Index, Info, kPassCount>> &&mesh_handle_ptr) {
		return std::make_unique<MeshEraser>(std::move(mesh_handle_ptr));
	}
	static inline std::unique_ptr<MeshEraser>
	Create(std::vector<std::unique_ptr<MeshHandle<Vertex, Index, Info, kPassCount>>> &&mesh_handle_vector) {
		return std::make_unique<MeshEraser>(std::move(mesh_handle_vector));
	}
	inline void Run() override {
		if (m_mesh_handle_vector.empty())
			return;
		std::set<std::shared_ptr<MeshCluster<Vertex, Index, Info, kPassCount>>> cluster_set;
		for (auto &i : m_mesh_handle_vector) {
			if (!i)
				continue;
			i->destroy();
			// cluster_set.insert(std::move(i->m_cluster_ptr));
		}
		/* for (auto &i : cluster_set) {
			i->upload_mesh_info_vector();
		} */
	}
	~MeshEraser() override = default;
};

#endif
