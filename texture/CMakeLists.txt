cmake_minimum_required(VERSION 3.15)
project(HyperCraft)

set(CMAKE_CXX_STANDARD 17)

if (NOT TARGET hc::dep)
    add_subdirectory(../dep ../dep)
endif ()

file(GLOB BLOCK_TEXTURES image/block/*.png)
file(GLOB MISC_TEXTURES image/misc/*.png)

set(GENERATED_DIR ${CMAKE_CURRENT_SOURCE_DIR}/internal/generated)

add_subdirectory(dep)

find_package(PNG)
if (PNG_FOUND)
    add_executable(HyperCraft_block_tex_gen generator/block_tex_gen.cpp register/block_textures)
    target_link_libraries(HyperCraft_block_tex_gen PRIVATE hc::texture::dep hc::dep PNG::PNG)
    target_include_directories(HyperCraft_block_tex_gen PRIVATE internal)

    add_executable(HyperCraft_misc_tex_gen generator/misc_tex_gen.cpp)
    target_link_libraries(HyperCraft_misc_tex_gen PRIVATE hc::texture::dep hc::dep PNG::PNG)

    add_custom_command(
            OUTPUT ${GENERATED_DIR}/block_texture_png.inl
            OUTPUT ${GENERATED_DIR}/block_texture_transparency.inl
            COMMAND $<TARGET_FILE_DIR:HyperCraft_block_tex_gen>/HyperCraft_block_tex_gen ${GENERATED_DIR}/block_texture_png.inl ${GENERATED_DIR}/block_texture_transparency.inl
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/image/block/
            DEPENDS HyperCraft_block_tex_gen ${BLOCK_TEXTURES}
    )

    add_custom_command(
            OUTPUT ${GENERATED_DIR}/misc_texture_png.inl
            COMMAND $<TARGET_FILE_DIR:HyperCraft_misc_tex_gen>/HyperCraft_misc_tex_gen ${GENERATED_DIR}/misc_texture_png.inl
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/image/misc/
            DEPENDS HyperCraft_misc_tex_gen ${MISC_TEXTURES}
    )
endif ()

add_library(HyperCraft_texture_block INTERFACE
        include/texture/BlockTexture.hpp
        ${GENERATED_DIR}/block_texture_png.inl
        ${GENERATED_DIR}/block_texture_transparency.inl
        )
add_library(hc::texture::block ALIAS HyperCraft_texture_block)
target_include_directories(HyperCraft_texture_block INTERFACE include internal)

add_library(HyperCraft_texture_misc INTERFACE
        include/texture/MiscTexture.hpp
        ${GENERATED_DIR}/misc_texture_png.inl
        )
add_library(hc::texture::misc ALIAS HyperCraft_texture_misc)
target_include_directories(HyperCraft_texture_misc INTERFACE include internal)

add_library(HyperCraft_texture_loader INTERFACE)
add_library(hc::texture::loader ALIAS HyperCraft_texture_loader)
target_link_libraries(HyperCraft_texture_loader INTERFACE hc::texture::dep)

add_library(HyperCraft_texture INTERFACE)
add_library(hc::texture ALIAS HyperCraft_texture)
target_link_libraries(HyperCraft_texture INTERFACE hc::texture::block hc::texture::misc hc::texture::loader)

