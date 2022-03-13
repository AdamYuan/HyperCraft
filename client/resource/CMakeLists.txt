cmake_minimum_required(VERSION 3.15)
project(CubeCraft3)

set(BLOCK_TEXTURE_NAME
        stone.png
        dirt.png
        grass_top.png
        grass_side.png
        sand.png
        log_top.png
        log_side.png
        plank.png
        glass.png
        snow.png
        blue_ice.png
        sandstone.png
        water.png
        )

set(BLOCK_TEXTURE_PATH)
foreach (X IN LISTS BLOCK_TEXTURE_NAME)
    list(APPEND BLOCK_TEXTURE_PATH texture/block/${X})
endforeach ()

if (NOT TARGET CubeCraft3_common)
    add_subdirectory(../../common ../../common)
endif ()
add_subdirectory(dep)

add_executable(client_block_tex_gen generator/block_tex_gen.cpp)
target_link_libraries(client_block_tex_gen PRIVATE CubeCraft3_common_header client_resource_dep)

add_custom_command(OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/include/client/texture/block_texture_png.inl
        COMMAND client_block_tex_gen ../../include/client/texture/block_texture_png.inl
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/texture/block/
        DEPENDS client_block_tex_gen ${BLOCK_TEXTURE_PATH}
        )

add_custom_target(
        client_resource_inl ALL DEPENDS
        include/client/texture/block_texture_png.inl
)
add_library(client_resource INTERFACE)
add_dependencies(client_resource client_resource_inl)
target_include_directories(client_resource INTERFACE include)
target_link_libraries(client_resource INTERFACE client_resource_dep)
