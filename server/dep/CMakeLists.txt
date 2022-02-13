cmake_minimum_required(VERSION 3.15)
project(CubeCraft3)

set(CMAKE_CXX_STANDARD 14)

add_library(server_dep INTERFACE
        )

#target_link_libraries(server_dep PRIVATE ${CMAKE_DL_LIBS})
#target_include_directories(server_dep PUBLIC .)
