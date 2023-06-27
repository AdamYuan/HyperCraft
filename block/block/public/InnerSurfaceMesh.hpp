#pragma once

#include <block/BlockMesh.hpp>

namespace hc::block {

inline static constexpr BlockMesh InnerSurfaceMesh(BlockTexID tex_id, BlockFace face, uint8_t dist) {
	uint8_t axis = face >> 1, u = (axis + 1) % 3, v = (axis + 2) % 3;
	uint8_t du[3] = {0}, dv[3] = {0}, x[3] = {0};
	if (face & 1u) {
		du[v] = dv[u] = 16;
		x[axis] = 16 - (int32_t)dist;
	} else {
		dv[v] = du[u] = 16;
		x[axis] = dist;
	}

	return {{
	            {axis,
	             face,
	             face,
	             {tex_id},
	             {
	                 {x[0], x[1], x[2]},
	                 {x[0] + du[0], x[1] + du[1], x[2] + du[2]},
	                 {x[0] + du[0] + dv[0], x[1] + du[1] + dv[1], x[2] + du[2] + dv[2]},
	                 {x[0] + dv[0], x[1] + dv[1], x[2] + dv[2]},
	             }},
	            {axis,
	             face,
	             (BlockFace)(face ^ 1u),
	             {tex_id},
	             {
	                 {x[0], x[1], x[2]},
	                 {x[0] + dv[0], x[1] + dv[1], x[2] + dv[2]},
	                 {x[0] + du[0] + dv[0], x[1] + du[1] + dv[1], x[2] + du[2] + dv[2]},
	                 {x[0] + du[0], x[1] + du[1], x[2] + du[2]},
	             }},
	        },
	        BlockTexture{tex_id}.UseTransparentPass() ? 1u : 2u,
	        {{{x[0], x[1], x[2]}, {x[0] + du[0] + dv[0], x[1] + du[1] + dv[1], x[2] + du[2] + dv[2]}}},
	        1u};
}

template <BlockTexID TexID, BlockFace Face, uint8_t Dist>
inline static constexpr BlockMesh kInnerSurfaceMesh = InnerSurfaceMesh(TexID, Face, Dist);

} // namespace hc::block