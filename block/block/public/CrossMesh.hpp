#pragma once

#include <block/BlockMesh.hpp>

namespace hc::block {

template <BlockTexID TexID, int Radius, int Low, int High, BlockFace LightFace = BlockFaces::kTop>
inline static constexpr BlockMesh kCrossMesh = {{
	                                                {0,
	                                                 LightFace,
	                                                 BlockFaces::kLeft,
	                                                 {TexID},
	                                                 {
		                                                 {8 - Radius, Low, 8 - Radius},
		                                                 {8 + Radius, Low, 8 + Radius},
		                                                 {8 + Radius, High, 8 + Radius},
		                                                 {8 - Radius, High, 8 - Radius},
	                                                 }},
	                                                {2,
	                                                 LightFace,
	                                                 BlockFaces::kBack,
	                                                 {TexID, BlockTextures::kTransNegU},
	                                                 {
		                                                 {8 - Radius, Low, 8 + Radius},
		                                                 {8 + Radius, Low, 8 - Radius},
		                                                 {8 + Radius, High, 8 - Radius},
		                                                 {8 - Radius, High, 8 + Radius},
	                                                 }},
	                                                {0,
	                                                 LightFace,
	                                                 BlockFaces::kRight,
	                                                 {TexID},
	                                                 {
		                                                 {8 - Radius, High, 8 - Radius},
		                                                 {8 + Radius, High, 8 + Radius},
		                                                 {8 + Radius, Low, 8 + Radius},
		                                                 {8 - Radius, Low, 8 - Radius},
	                                                 }},
	                                                {2,
	                                                 LightFace,
	                                                 BlockFaces::kFront,
	                                                 {TexID, BlockTextures::kTransNegU},
	                                                 {
		                                                 {8 - Radius, High, 8 + Radius},
		                                                 {8 + Radius, High, 8 - Radius},
		                                                 {8 + Radius, Low, 8 - Radius},
		                                                 {8 - Radius, Low, 8 + Radius},
	                                                 }},
                                                },
                                                BlockTexture{TexID}.UseTransparentPass() ? 2u : 4u,
                                                {{{8 - Radius, Low, 8 - Radius}, {8 + Radius, High, 8 + Radius}}},
                                                1};

}
