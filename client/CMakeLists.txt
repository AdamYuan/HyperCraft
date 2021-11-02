cmake_minimum_required(VERSION 3.15)
project(CubeCraft3)

set(CMAKE_CXX_STANDARD 17)

add_subdirectory(dep)
add_subdirectory(shader)
add_subdirectory(resource)
if (NOT TARGET CubeCraft3_common)
    add_subdirectory(../common ../common)
endif ()

add_executable(CubeCraft3_client
        src/main.cpp
        include/client/Config.hpp
        src/Application.cpp
        include/client/Application.hpp
        src/Chunk.cpp
        include/client/Chunk.hpp
        src/World.cpp
        include/client/World.hpp
        src/ChunkWorker.cpp
        include/client/ChunkWorker.hpp
        src/ChunkMesher.cpp
        include/client/ChunkMesher.hpp
        src/WorldRenderer.cpp
        include/client/WorldRenderer.hpp
        src/Camera.cpp
        include/client/Camera.hpp
        src/GlobalTexture.cpp
        include/client/GlobalTexture.hpp
        src/Canvas.cpp
        include/client/Canvas.hpp
        src/ENetClient.cpp
        include/client/ENetClient.hpp
        src/ClientBase.cpp include/client/ClientBase.hpp src/LocalClient.cpp include/client/LocalClient.hpp)

target_link_libraries(CubeCraft3_client PRIVATE client_dep client_shader client_resource CubeCraft3_common)
target_include_directories(CubeCraft3_client PRIVATE include)
