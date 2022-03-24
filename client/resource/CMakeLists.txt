cmake_minimum_required(VERSION 3.15)
project(CubeCraft3)

file(GLOB BLOCK_TEXTURES texture/block/*.png)
file(GLOB MISC_TEXTURES texture/misc/*.png)

if (NOT TARGET CubeCraft3_common)
    add_subdirectory(../../common ../../common)
endif ()
add_subdirectory(dep)

find_package(PNG)
if (PNG_FOUND)
    add_executable(client_block_tex_gen generator/block_tex_gen.cpp)
    target_link_libraries(client_block_tex_gen PRIVATE CubeCraft3_common_header client_resource_dep PNG::PNG)

    add_executable(client_misc_tex_gen generator/misc_tex_gen.cpp)
    target_link_libraries(client_misc_tex_gen PRIVATE client_resource_dep PNG::PNG)

    add_custom_command(
            OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/include/client/texture/block_texture_png.inl
            OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/include/client/texture/block_texture_transparency.inl
            COMMAND ${CMAKE_CURRENT_BINARY_DIR}/client_block_tex_gen ../../include/client/texture/block_texture_png.inl ../../include/client/texture/block_texture_transparency.inl
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/texture/block/
            DEPENDS client_block_tex_gen ${BLOCK_TEXTURES}
    )

    add_custom_command(
            OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/include/client/texture/misc_texture_png.inl
            COMMAND ${CMAKE_CURRENT_BINARY_DIR}/client_misc_tex_gen ../../include/client/texture/misc_texture_png.inl
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/texture/misc/
            DEPENDS client_misc_tex_gen ${MISC_TEXTURES}
    )
endif ()

add_library(client_resource INTERFACE
        include/client/texture/block_texture_png.inl
        include/client/texture/block_texture_transparency.inl

        include/client/texture/misc_texture_png.inl
        )
target_include_directories(client_resource INTERFACE include)
target_link_libraries(client_resource INTERFACE client_resource_dep)
