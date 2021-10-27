cmake_minimum_required(VERSION 3.15)
project(CubeCraft3)

set(CMAKE_CXX_STANDARD 14)

include(TestBigEndian)
TEST_BIG_ENDIAN(IS_BIG_ENDIAN)
if (IS_BIG_ENDIAN)
    ADD_DEFINITIONS(-DBIG_ENDIAN)
    message(STATUS "Endianness: Big")
else ()
    ADD_DEFINITIONS(-DLITTLE_ENDIAN)
    message(STATUS "Endianness: Little")
endif ()

add_subdirectory(dep)

add_library(CubeCraft3_common STATIC
        src/Block.cpp
        include/common/Block.hpp
        src/Light.cpp
        include/common/Light.hpp
        src/Message.cpp
        include/common/Message.hpp
        )

find_package(Threads REQUIRED)
target_link_libraries(CubeCraft3_common PUBLIC common_dep Threads::Threads)
target_include_directories(CubeCraft3_common PUBLIC include)
