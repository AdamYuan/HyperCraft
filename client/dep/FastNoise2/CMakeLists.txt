# CMakeList.txt : CMake project for FastNoise2
cmake_minimum_required(VERSION 3.7.1)

project(FastNoise2 VERSION 0.9.7)
set(CMAKE_CXX_STANDARD 17)

message("FastNoise2 Arch: ${CMAKE_SYSTEM_PROCESSOR}")

if (CMAKE_SYSTEM_PROCESSOR MATCHES armv7)

    set(FASTSIMD_COMPILE_ARMV7 true)
    set(FASTSIMD_COMPILE_ARM true)
    set(FASTSIMD_COMPILE_HAVE_NEON true)

elseif ((CMAKE_SYSTEM_PROCESSOR STREQUAL aarch64) OR (CMAKE_SYSTEM_PROCESSOR STREQUAL arm64))

    set(FASTSIMD_COMPILE_AARCH64 true)
    set(FASTSIMD_COMPILE_ARM true)
    set(FASTSIMD_COMPILE_HAVE_NEON true)

elseif (CMAKE_SYSTEM_PROCESSOR MATCHES arm)

    set(FASTSIMD_COMPILE_ARM true)

endif ()

if (MSVC)
    #setup pdb target location
    set(pdb_output_dir "${CMAKE_CURRENT_BINARY_DIR}/pdb-files")

    set(CMAKE_PDB_OUTPUT_DIRECTORY "${pdb_output_dir}")
    set(CMAKE_COMPILE_PDB_OUTPUT_DIRECTORY "${pdb_output_dir}")

    #need to sync pdb files
    add_compile_options("/FS")
endif ()

add_subdirectory(src)
