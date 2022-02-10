cmake_minimum_required(VERSION 3.15)
project(CubeCraft3)

if (CMAKE_SYSTEM_NAME STREQUAL Windows)
	add_definitions(-D_USE_MATH_DEFINES)
	add_definitions(-DNOMINMAX)
	add_definitions(-DWIN32_LEAN_AND_MEAN)
endif ()

add_subdirectory(client)
add_subdirectory(server)
