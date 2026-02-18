# CMakeLists.txt for gen3dart standalone OpenGL project
cmake_minimum_required(VERSION 3.10)
project(gen3dart)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/../bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/../bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/../bin)

# Generate root_directory.h from template (used by learnopengl/filesystem.h)
configure_file(
    ${CMAKE_SOURCE_DIR}/configuration/root_directory.h.in
    ${CMAKE_SOURCE_DIR}/includes/learnopengl/root_directory.h
)

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/includes)

# Find OpenGL
find_package(OpenGL REQUIRED)

# GLFW library
set(GLFW_LIB ${CMAKE_SOURCE_DIR}/lib/glfw3.lib)

# Source files
set(GLAD_SRC ${CMAKE_SOURCE_DIR}/src/glad.c)

# kinetic_sculpture executable
add_executable(kinetic_sculpture
    src/kinetic_sculpture/kinetic_sculpture.cpp
    src/stb_image.cpp
    ${GLAD_SRC}
)

target_link_libraries(kinetic_sculpture
    ${OPENGL_LIBRARIES}
    ${GLFW_LIB}
)

# Windows-specific settings
if(WIN32)
    target_link_libraries(kinetic_sculpture
        user32
        gdi32
        shell32
    )
    # Suppress LNK4099 warnings about missing PDB files for pre-compiled libraries
    target_link_options(kinetic_sculpture PRIVATE "/ignore:4099")
endif()
