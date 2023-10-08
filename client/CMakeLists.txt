cmake_minimum_required(VERSION 3.15)
project(HyperCraft)

set(CMAKE_CXX_STANDARD 20)

add_subdirectory(dep)
add_subdirectory(shader)
if (NOT TARGET hc::texture)
    add_subdirectory(../texture ../texture)
endif ()
if (NOT TARGET hc::common)
    add_subdirectory(../common ../common)
endif ()

add_executable(HyperCraft_client
        src/main.cpp
        src/Application.cpp
        src/Chunk.cpp
        src/World.cpp
        src/ChunkWorkerBase.cpp
        src/ChunkMesher.cpp
        src/WorldRenderer.cpp
        src/Camera.cpp
        src/GlobalTexture.cpp
        src/ENetClient.cpp
        src/LocalClient.cpp
        src/ChunkGenerator.cpp
        src/ChunkGenerateTask.cpp
        src/DefaultTerrain.cpp
        src/ChunkLighter.cpp
)

target_link_libraries(HyperCraft_client
        PRIVATE hc::client::dep hc::client::shader hc::texture hc::common)
target_include_directories(HyperCraft_client PRIVATE include)
