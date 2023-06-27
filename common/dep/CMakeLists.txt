cmake_minimum_required(VERSION 3.15)
project(HyperCraft)

set(CMAKE_CXX_STANDARD 20)

include_directories(.)

add_subdirectory(spdlog)
add_subdirectory(enet)

add_library(common_dep STATIC
        concurrentqueue.h
        lightweightsemaphore.h
        blockingconcurrentqueue.h

        sqlite3.c
        sqlite3.h
        sqlite3ext.h
        )
add_library(hc::common::dep ALIAS common_dep)

target_compile_definitions(common_dep PUBLIC GLM_FORCE_SWIZZLE)
target_include_directories(common_dep PUBLIC .)
target_link_libraries(common_dep PRIVATE ${CMAKE_DL_LIBS} PUBLIC spdlog enet)
