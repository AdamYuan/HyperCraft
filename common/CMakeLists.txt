cmake_minimum_required(VERSION 3.15)
project(CubeCraft3)

set(CMAKE_CXX_STANDARD 17)

add_library(CubeCraft3_common_header INTERFACE)
target_include_directories(CubeCraft3_common_header INTERFACE include dep)

add_library(CubeCraft3_common STATIC
        include/common/Endian.hpp
        include/common/Block.hpp
        include/common/Light.hpp
        include/common/Message.hpp
        include/common/ByteStream.hpp
        src/WorldDatabase.cpp
        include/common/WorldDatabase.hpp
        include/common/AABB.hpp
        include/common/Position.hpp
        include/common/Biome.hpp include/common/Size.hpp)

include(TestBigEndian)
TEST_BIG_ENDIAN(IS_BIG_ENDIAN)
if (IS_BIG_ENDIAN)
    target_compile_definitions(CubeCraft3_common_header INTERFACE IS_BIG_ENDIAN)
    target_compile_definitions(CubeCraft3_common PUBLIC IS_BIG_ENDIAN)
    message(STATUS "Endianness: Big")
endif ()

add_subdirectory(dep)
find_package(Threads REQUIRED)

target_link_libraries(CubeCraft3_common PUBLIC common_dep Threads::Threads)
target_include_directories(CubeCraft3_common PUBLIC include)
