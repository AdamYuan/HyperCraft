cmake_minimum_required(VERSION 3.15)
project(CubeCraft3)

set(CMAKE_CXX_STANDARD 17)

add_subdirectory(FastNoise2)
add_subdirectory(MyVK)

add_library(client_dep INTERFACE)
target_link_libraries(client_dep INTERFACE FastNoise myvk::vulkan myvk::glfw myvk::imgui myvk::rg)
