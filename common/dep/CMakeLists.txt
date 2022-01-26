cmake_minimum_required(VERSION 3.15)
project(CubeCraft3)

set(CMAKE_CXX_STANDARD 14)

# Windows' math include does not define constants by default.
# Set this definition so it does.
# Also set NOMINMAX so the min and max functions are not overwritten with macros.
if (CMAKE_SYSTEM_NAME STREQUAL Windows)
    add_definitions(-D_USE_MATH_DEFINES)
    add_definitions(-DNOMINMAX)
ENDIF ()

include_directories(.)

add_subdirectory(spdlog)
add_subdirectory(enet)

add_library(common_dep STATIC
        concurrentqueue.h
        lightweightsemaphore.h
        blockingconcurrentqueue.h

        atomic_shared_ptr.hpp
        sqlite3.c
        sqlite3.h
        sqlite3ext.h
        )

target_compile_definitions(common_dep PUBLIC GLM_FORCE_SWIZZLE)
target_include_directories(common_dep PUBLIC .)
target_link_libraries(common_dep PRIVATE ${CMAKE_DL_LIBS} PUBLIC spdlog enet)
