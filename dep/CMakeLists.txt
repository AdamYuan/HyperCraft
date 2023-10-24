cmake_minimum_required(VERSION 3.15)
project(HyperCraft)

set(CMAKE_CXX_STANDARD 20)

add_library(HyperCraft_dep INTERFACE)
add_library(hc::dep ALIAS HyperCraft_dep)

target_compile_definitions(HyperCraft_dep INTERFACE GLM_FORCE_SWIZZLE GLM_ENABLE_EXPERIMENTAL)
target_include_directories(HyperCraft_dep INTERFACE .)
