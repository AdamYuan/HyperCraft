cmake_minimum_required(VERSION 3.15)
project(HyperCraft)

set(CMAKE_CXX_STANDARD 20)

if (NOT TARGET hc::block)
    add_subdirectory(../block ../block)
endif ()

if (NOT TARGET hc::dep)
    add_subdirectory(../dep ../dep)
endif ()

add_library(HyperCraft_common STATIC
        src/WorldDatabase.cpp
        )
add_library(hc::common ALIAS HyperCraft_common)

target_include_directories(HyperCraft_common INTERFACE include dep)

include(TestBigEndian)
TEST_BIG_ENDIAN(IS_BIG_ENDIAN)
if (IS_BIG_ENDIAN)
    target_compile_definitions(HyperCraft_common INTERFACE IS_BIG_ENDIAN)
    target_compile_definitions(HyperCraft_common PUBLIC IS_BIG_ENDIAN)
    message(STATUS "Endianness: Big")
endif ()

add_subdirectory(dep)
find_package(Threads REQUIRED)

target_link_libraries(HyperCraft_common
        PUBLIC hc::common::dep hc::block hc::dep Threads::Threads
        )
target_include_directories(HyperCraft_common PUBLIC include)
