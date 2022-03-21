# CMakeList.txt : CMake project for FastNoise2
cmake_minimum_required(VERSION 3.7.1)

project(FastNoise2 VERSION 0.9.4)
set(CMAKE_CXX_STANDARD 17)

# Build DLL
set(BUILD_SHARED_LIBS OFF) 
option(FASTNOISE2_NOISETOOL "Build NoiseTool application" OFF)
option(FASTNOISE2_TESTS "Build tests" OFF)

if(MSVC)
    #setup pdb target location
    set(pdb_output_dir "${CMAKE_CURRENT_BINARY_DIR}/pdb-files")

    set(CMAKE_PDB_OUTPUT_DIRECTORY "${pdb_output_dir}")
    set(CMAKE_COMPILE_PDB_OUTPUT_DIRECTORY "${pdb_output_dir}") 

    #need to sync pdb files
    add_compile_options("/FS")
endif()

# Introduce variables:
# * CMAKE_INSTALL_LIBDIR
# * CMAKE_INSTALL_BINDIR
include(GNUInstallDirs) 
set(install_targets "")

add_subdirectory(src)

if(FASTNOISE2_NOISETOOL)
    include(cmake/CPM.cmake)
    add_subdirectory(NoiseTool)
endif()

if(FASTNOISE2_TESTS)
    include(cmake/CPM.cmake)  
    add_subdirectory(tests)
endif()

