cmake_minimum_required(VERSION 3.20)
project(gic)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

add_subdirectory(deps/cglm)
add_subdirectory(deps/SDL)

add_executable(gic
    src/main.c
    src/util.c

    src/graphics/camera.c
    src/graphics/renderer.c
    src/graphics/shader.c

    src/platform/sdl_window.c
    
    # GLAD - OpenGL Loader
    deps/glad/src/gl.c
    deps/glad/src/wgl.c)

target_link_libraries(gic PRIVATE opengl32 dwmapi cglm SDL3-shared)
target_include_directories(gic PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/deps/glad/include ${CMAKE_CURRENT_SOURCE_DIR}/src)

configure_file(src/graphics/shaders/voxel.vert ${CMAKE_BINARY_DIR}/bin/shaders/voxel.vert COPYONLY)
configure_file(src/graphics/shaders/voxel.frag ${CMAKE_BINARY_DIR}/bin/shaders/voxel.frag COPYONLY)
