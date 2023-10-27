#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#include <block/Block.hpp>
#include <limits>
#include <string>

using namespace hc::block;

TEST_CASE("BlockID = 0 is Air") {
	for (BlockMeta variant = 0; variant < std::numeric_limits<BlockMeta>::max(); ++variant)
		for (BlockMeta transform = 0; transform < std::numeric_limits<BlockMeta>::max(); ++transform) {
			CHECK(Block(0, variant, transform).GetName() != nullptr);            // Must have a name

			CHECK(Block(0, variant, transform).GetName() == std::string{"Air"}); // Name must be "Air"
			CHECK(Block(0, variant, transform).GetCollisionMask() ==
			      BlockCollision::kNone);                                    // Must not have collision
			CHECK(Block(0, variant, transform).GetIndirectLightPass() ==
			      true); // Indirect light must be able to pass through
			CHECK(Block(0, variant, transform).GetVerticalLightPass() ==
			      true); // Vertical light must be able to pass through
			CHECK(Block(0, variant, transform).GetCustomMesh() == nullptr); // Must not have custom mesh
		}
}

TEST_CASE("Block Custom Meshes") {
	for (BlockID id = 0; id < std::numeric_limits<BlockID>::max(); ++id)
		for (BlockMeta variant = 0; variant < std::numeric_limits<BlockMeta>::max(); ++variant)
			for (BlockMeta transform = 0; transform < std::numeric_limits<BlockMeta>::max(); ++transform) {
				Block block{id, variant, transform};
				if (block.HaveCustomMesh()) {
					CHECK(block.GetCustomMesh() != nullptr);       // Must have custom mesh

					CHECK(block.GetCustomMesh()->face_count != 0); // The custom mesh must not be empty
					CHECK(block.GetCustomMesh()->face_count <=
					      kBlockMeshMaxFaceCount);                 // Face count should not exceed the limit

					CHECK(block.GetCustomMesh()->aabb_count <=
					      kBlockMeshMaxAABBCount); // AABB count should not exceed the limit

					for (uint32_t f = 0; f < block.GetCustomMesh()->face_count; ++f) {
						const auto &face = block.GetCustomMesh()->faces[f];
						CHECK(!face.texture.Empty());                // Custom Mesh Faces should not be empty
						CHECK(face.axis == (face.render_face >> 1)); // Axis must match Render Face
					}
				} else
					CHECK(block.GetCustomMesh() == nullptr); // Must not have custom mesh
			}
}

TEST_CASE("Block Name != nullptr") {
	for (BlockID id = 0; id < std::numeric_limits<BlockID>::max(); ++id)
		for (BlockMeta variant = 0; variant < std::numeric_limits<BlockMeta>::max(); ++variant)
			for (BlockMeta transform = 0; transform < std::numeric_limits<BlockMeta>::max(); ++transform) {
				CHECK(Block(id, variant, transform).GetName() != nullptr); // Name must not be null
			}
}

TEST_CASE("Block Lighting") {
	for (BlockID id = 0; id < std::numeric_limits<BlockID>::max(); ++id)
		for (BlockMeta variant = 0; variant < std::numeric_limits<BlockMeta>::max(); ++variant)
			for (BlockMeta transform = 0; transform < std::numeric_limits<BlockMeta>::max(); ++transform) {
				Block block{id, variant, transform};
				if (block.GetVerticalLightPass())
					CHECK(block.GetIndirectLightPass()); // Blocks allow vertical light to pass through must allow
					                                     // indirect light to pass through
			}
}

TEST_CASE("BlockFaceOpposite()") {
	CHECK(BlockFaceOpposite(BlockFaces::kRight) == BlockFaces::kLeft); // The opposite of Right is Left
	CHECK(BlockFaceOpposite(BlockFaces::kLeft) == BlockFaces::kRight); // The opposite of Left is Right

	CHECK(BlockFaceOpposite(BlockFaces::kFront) == BlockFaces::kBack); // The opposite of Front is Back
	CHECK(BlockFaceOpposite(BlockFaces::kBack) == BlockFaces::kFront); // The opposite of Back is Front

	CHECK(BlockFaceOpposite(BlockFaces::kTop) == BlockFaces::kBottom); // The opposite of Top is Bottom
	CHECK(BlockFaceOpposite(BlockFaces::kBottom) == BlockFaces::kTop); // The opposite of Bottom is Top
}

TEST_CASE("BlockFaceProceed()") {
	CHECK(BlockFaceProceed(glm::i32vec3{0, 0, 0}, BlockFaces::kRight) ==
	      glm::i32vec3{1, 0, 0});            // Check proceed to the right
	CHECK(BlockFaceProceed(glm::i32vec3{123120312, 77, 23}, BlockFaces::kLeft) ==
	      glm::i32vec3{123120311, 77, 23});  // Check proceed to the left
	CHECK(BlockFaceProceed(glm::i32vec3{-1231, -77, 23}, BlockFaces::kTop) ==
	      glm::i32vec3{-1231, -76, 23});     // Check proceed to the top
	CHECK(BlockFaceProceed(glm::i32vec3{-123, -7, 2}, BlockFaces::kBottom) ==
	      glm::i32vec3{-123, -8, 2});        // Check proceed to the bottom
	CHECK(BlockFaceProceed(glm::i32vec3{72334, -3245, 0x23}, BlockFaces::kFront) ==
	      glm::i32vec3{72334, -3245, 0x24}); // Check proceed to the front
	CHECK(BlockFaceProceed(glm::i32vec3{1424, -245, 0x234}, BlockFaces::kBack) ==
	      glm::i32vec3{1424, -245, 0x233});  // Check proceed to the back
}
