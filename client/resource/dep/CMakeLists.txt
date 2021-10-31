cmake_minimum_required(VERSION 3.15)
project(CubeCraft3)

add_library(client_resource_dep STATIC
        stb_image.cpp
        stb_image_write.cpp
        )
target_include_directories(client_resource_dep PUBLIC .)
