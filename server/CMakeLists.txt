cmake_minimum_required(VERSION 3.15)
project(CubeCraft3)

set(CMAKE_CXX_STANDARD 14)

add_subdirectory(dep)
if (NOT TARGET CubeCraft3_common)
    add_subdirectory(../common ../common)
endif ()

add_executable(CubeCraft3_server
        src/main.cpp
        src/ENetServer.cpp
        include/server/ENetServer.hpp
        )

target_link_libraries(CubeCraft3_server PRIVATE server_dep CubeCraft3_common)
target_include_directories(CubeCraft3_server PRIVATE include)
