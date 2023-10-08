cmake_minimum_required(VERSION 3.15)
project(HyperCraft)

add_subdirectory(libcuckoo)
add_subdirectory(FastNoise2)

set(MYVK_TESTING OFF)
add_subdirectory(MyVK)

add_library(client_dep INTERFACE)
add_library(hc::client::dep ALIAS client_dep)
target_link_libraries(client_dep INTERFACE libcuckoo FastNoise myvk::vulkan myvk::glfw myvk::imgui myvk::rg)
