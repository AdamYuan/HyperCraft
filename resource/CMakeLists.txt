cmake_minimum_required(VERSION 3.15)
project(CubeCraft3)

file(GLOB BLOCK_TEXTURES texture/block/*.png)
file(GLOB MISC_TEXTURES texture/misc/*.png)

set(GENERATED_DIR ${CMAKE_CURRENT_SOURCE_DIR}/internal/generated)

add_subdirectory(dep)

find_package(PNG)
if (PNG_FOUND)
    add_executable(CubeCraft3_block_tex_gen generator/block_tex_gen.cpp)
    target_link_libraries(CubeCraft3_block_tex_gen PRIVATE resource_dep PNG::PNG)
    target_include_directories(CubeCraft3_block_tex_gen PRIVATE internal)

    add_executable(CubeCraft3_misc_tex_gen generator/misc_tex_gen.cpp)
    target_link_libraries(CubeCraft3_misc_tex_gen PRIVATE resource_dep PNG::PNG)

    add_custom_command(
            OUTPUT ${GENERATED_DIR}/block_texture_png.inl
            OUTPUT ${GENERATED_DIR}/block_texture_transparency.inl
            COMMAND ${CMAKE_CURRENT_BINARY_DIR}/CubeCraft3_block_tex_gen ${GENERATED_DIR}/block_texture_png.inl ${GENERATED_DIR}/block_texture_transparency.inl
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/texture/block/
            DEPENDS CubeCraft3_block_tex_gen ${BLOCK_TEXTURES}
    )

    add_custom_command(
            OUTPUT ${GENERATED_DIR}/misc_texture_png.inl
            COMMAND ${CMAKE_CURRENT_BINARY_DIR}/CubeCraft3_misc_tex_gen ${GENERATED_DIR}/misc_texture_png.inl
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/texture/misc/
            DEPENDS CubeCraft3_misc_tex_gen ${MISC_TEXTURES}
    )
endif ()

add_library(CubeCraft3_resource_header INTERFACE
        include/resource/texture/BlockTexture.hpp
        ${GENERATED_DIR}/block_texture_png.inl
        ${GENERATED_DIR}/block_texture_transparency.inl
        ${GENERATED_DIR}/misc_texture_png.inl
        include/resource/texture/MiscTexture.hpp)
target_include_directories(CubeCraft3_resource_header INTERFACE include internal)

add_library(CubeCraft3_resource INTERFACE)
target_link_libraries(CubeCraft3_resource INTERFACE CubeCraft3_resource_header resource_dep)

