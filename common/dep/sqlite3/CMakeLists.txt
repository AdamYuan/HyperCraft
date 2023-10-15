cmake_minimum_required(VERSION 3.15)

add_library(sqlite3 STATIC src/sqlite3.c)
target_include_directories(sqlite3 PUBLIC include)
target_link_libraries(sqlite3 PRIVATE ${CMAKE_DL_LIBS})

