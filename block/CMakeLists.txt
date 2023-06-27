cmake_minimum_required(VERSION 3.15)
project(HyperCraft)

set(CMAKE_CXX_STANDARD 20)

if (NOT TARGET hc::dep)
    add_subdirectory(../dep ../dep)
endif ()
if (NOT TARGET hc::texture::block)
    add_subdirectory(../texture ../texture)
endif ()

file(GLOB BLOCK_DEFS block/*.hpp block/public/*)

set(GENERATED_DIR ${CMAKE_CURRENT_BINARY_DIR})

add_executable(HyperCraft_block_gen
        generator/block_gen.cpp
        )
target_link_libraries(HyperCraft_block_gen PRIVATE hc::texture::block hc::dep)

add_custom_command(
        OUTPUT ${GENERATED_DIR}/Block.cpp
        COMMAND $<TARGET_FILE_DIR:HyperCraft_block_gen>/HyperCraft_block_gen ${GENERATED_DIR}/Block.cpp
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/generator
        DEPENDS HyperCraft_block_gen generator/Block.cpp.in ${BLOCK_DEFS} register/blocks
)

add_library(HyperCraft_block STATIC
        ${GENERATED_DIR}/Block.cpp
        )
add_library(hc::block ALIAS HyperCraft_block)
target_include_directories(HyperCraft_block
        PUBLIC include
        PRIVATE block
        )
target_link_libraries(HyperCraft_block PUBLIC hc::texture::block hc::dep)

add_executable(HyperCraft_block_test test/test_block.cpp)
target_link_libraries(HyperCraft_block_test PRIVATE hc::block)