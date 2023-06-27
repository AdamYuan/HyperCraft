cmake_minimum_required(VERSION 3.15)
project(HyperCraft)

set(CMAKE_CXX_STANDARD 20)

add_library(server_dep INTERFACE)
add_library(hc::server::dep ALIAS server_dep)

#target_link_libraries(server_dep PRIVATE ${CMAKE_DL_LIBS})
#target_include_directories(server_dep PUBLIC .)
