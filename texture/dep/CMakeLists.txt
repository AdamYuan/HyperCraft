cmake_minimum_required(VERSION 3.15)
project(HyperCraft)

add_library(texture_dep STATIC
        stb_image.cpp
        )
add_library(hc::texture::dep ALIAS texture_dep)
target_include_directories(texture_dep PUBLIC .)
