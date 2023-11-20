cmake_minimum_required(VERSION 3.15)
project(HyperCraft)

set(CMAKE_CXX_STANDARD 20)

add_subdirectory(spdlog)
add_subdirectory(enet)
add_subdirectory(concurrentqueue)
add_subdirectory(lmdb)

add_library(common_dep INTERFACE)
add_library(hc::common::dep ALIAS common_dep)

target_compile_definitions(common_dep INTERFACE GLM_FORCE_SWIZZLE)
target_link_libraries(common_dep INTERFACE lmdb spdlog enet concurrentqueue)
