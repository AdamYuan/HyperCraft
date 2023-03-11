cmake_minimum_required(VERSION 3.15)
project(CubeCraft3)

set(SHADER_SRC
        chunk.vert
        chunk_opaque.frag
        chunk_transparent.frag
        chunk_culling.comp
        depth_hierarchy.frag
        quad.vert
        oit_compositor.frag
        post_process.frag
        )

find_program(GLSLC "glslc")
IF (GLSLC)
    function(add_spirv_shader INPUT_FILE)
        add_custom_command(OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/include/client/shader/${INPUT_FILE}.u32
                COMMAND glslc src/${INPUT_FILE} -o include/client/shader/${INPUT_FILE}.u32 -O -Os -mfmt=num
                WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/${INPUT_FILE}
                )
    endfunction()

    foreach (X IN LISTS SHADER_SRC)
        add_spirv_shader(${X})
    endforeach ()
ENDIF ()

set(SHADER_U32)
foreach (X IN LISTS SHADER_SRC)
    list(APPEND SHADER_U32 include/client/shader/${X}.u32)
endforeach ()

add_custom_target(
        client_shader_u32 ALL DEPENDS
        ${SHADER_U32}
)
add_library(client_shader INTERFACE)
add_dependencies(client_shader client_shader_u32)
target_include_directories(client_shader INTERFACE include)
