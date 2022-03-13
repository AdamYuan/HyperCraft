//
// Created by adamyuan on 1/25/22.
//

#ifndef CUBECRAFT3_COMMON_POSITION_HPP
#define CUBECRAFT3_COMMON_POSITION_HPP

#include <cinttypes>
#include <glm/glm.hpp>
#include <glm/gtx/hash.hpp>

using ChunkPos1 = int16_t;
using ChunkPos2 = glm::vec<2, ChunkPos1>;
using ChunkPos3 = glm::vec<3, ChunkPos1>;

#endif
