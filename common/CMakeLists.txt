cmake_minimum_required(VERSION 3.15)
project(CubeCraft3)

set(CMAKE_CXX_STANDARD 17)

include(TestBigEndian)
TEST_BIG_ENDIAN(IS_BIG_ENDIAN)
if (IS_BIG_ENDIAN)
    ADD_DEFINITIONS(-DIS_BIG_ENDIAN)
    message(STATUS "Endianness: Big")
endif ()

add_library(CubeCraft3_common_header INTERFACE)
target_include_directories(CubeCraft3_common_header INTERFACE include)

add_subdirectory(dep)
add_library(CubeCraft3_common STATIC
        include/common/Block.hpp
        include/common/Light.hpp
        include/common/Message.hpp
        include/common/ByteStream.hpp

        include/common/LevelDB.hpp
        src/LevelDB.cpp
        )
find_package(Threads REQUIRED)
target_link_libraries(CubeCraft3_common PUBLIC common_dep Threads::Threads)
target_include_directories(CubeCraft3_common PUBLIC include)
