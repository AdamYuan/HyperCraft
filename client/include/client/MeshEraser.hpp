#ifndef CUBECRAFT3_CLIENT_MESH_ERASER_HPP
#define CUBECRAFT3_CLIENT_MESH_ERASER_HPP

#include "client/mesh/MeshHandle.hpp"
#include <client/WorkerBase.hpp>
#include <set>
#include <utility>

#include <spdlog/spdlog.h>

template <typename Vertex, typename Index, typename Info> class MeshEraser : public WorkerBase {
private:
	std::vector<std::unique_ptr<MeshHandle<Vertex, Index, Info>>> m_mesh_handle_vector;

public:
	inline explicit MeshEraser(std::unique_ptr<MeshHandle<Vertex, Index, Info>> &&mesh_handle_ptr)
	    : m_mesh_handle_vector({std::move(mesh_handle_ptr)}) {}
	inline explicit MeshEraser(std::vector<std::unique_ptr<MeshHandle<Vertex, Index, Info>>> &&mesh_handle_vector)
	    : m_mesh_handle_vector{std::move(mesh_handle_vector)} {}

	static inline std::unique_ptr<MeshEraser>
	Create(std::unique_ptr<MeshHandle<Vertex, Index, Info>> &&mesh_handle_ptr) {
		return std::make_unique<MeshEraser>(std::move(mesh_handle_ptr));
	}
	static inline std::unique_ptr<MeshEraser>
	Create(std::vector<std::unique_ptr<MeshHandle<Vertex, Index, Info>>> &&mesh_handle_vector) {
		return std::make_unique<MeshEraser>(std::move(mesh_handle_vector));
	}
	inline void Run() override {
		if (m_mesh_handle_vector.empty())
			return;
		std::set<std::shared_ptr<MeshCluster<Vertex, Index, Info>>> cluster_set;
		/* for (auto &i : m_mesh_handle_vector) {
			if (!i)
				continue;
			i->destroy();
		} */
	}
	~MeshEraser() override = default;
};

#endif
