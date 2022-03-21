cmake_minimum_required(VERSION 3.15)
project(CubeCraft3)

file(GLOB BLOCK_TEXTURES texture/block/*.png)

if (NOT TARGET CubeCraft3_common)
    add_subdirectory(../../common ../../common)
endif ()
add_subdirectory(dep)

add_executable(client_block_tex_gen generator/block_tex_gen.cpp)
target_link_libraries(client_block_tex_gen PRIVATE CubeCraft3_common_header client_resource_dep)

add_custom_command(
        OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/include/client/texture/block_texture_png.inl
        OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/include/client/texture/block_texture_transparency.inl
        COMMAND ${CMAKE_CURRENT_BINARY_DIR}/client_block_tex_gen ../../include/client/texture/block_texture_png.inl ../../include/client/texture/block_texture_transparency.inl
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/texture/block/
        DEPENDS client_block_tex_gen ${BLOCK_TEXTURES}
        )

add_custom_target(
        client_resource_inl ALL DEPENDS
        ${CMAKE_CURRENT_SOURCE_DIR}/include/client/texture/block_texture_png.inl
        ${CMAKE_CURRENT_SOURCE_DIR}/include/client/texture/block_texture_transparency.inl
)
add_library(client_resource INTERFACE)
add_dependencies(client_resource client_resource_inl)
target_include_directories(client_resource INTERFACE include)
target_link_libraries(client_resource INTERFACE client_resource_dep)
