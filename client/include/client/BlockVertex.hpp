#pragma once

#include <block/Block.hpp>
#include <vector>

namespace hc::client {

struct BlockVertex { // Compressed mesh vertex for chunk
	static constexpr uint32_t kUnitBitOffset = 4u;
	static constexpr uint32_t kUnitOffset = 1u << kUnitBitOffset;
	// x, y, z, axis, texture id, texture transformation, face, AO, sunlight, torchlight
	uint32_t x10_y10_z10_axis2, tex10_trans3_face3_ao2_sl6_tl6;
	BlockVertex(uint32_t x10, uint32_t y10, uint32_t z10, uint8_t axis, block::BlockFace face, uint8_t ao,
	            uint8_t sunlight, uint8_t torchlight, texture::BlockTexID tex, texture::BlockTexTrans tex_trans)
	    : x10_y10_z10_axis2(x10 | (y10 << 10u) | (z10 << 20u) | (axis << 30u)),
	      tex10_trans3_face3_ao2_sl6_tl6((tex - 1u) | (tex_trans << 10u) | (face << 13u) | (ao << 16u) |
	                                     (sunlight << 18u) | (torchlight << 24u)) {}
};

struct BlockMesh {
	std::vector<BlockVertex> vertices;
	std::vector<uint16_t> indices;
	AABB<uint32_t> aabb{};
	bool transparent;
};

} // namespace hc::client