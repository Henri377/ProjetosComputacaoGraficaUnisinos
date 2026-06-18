cmake_minimum_required(VERSION 3.10)

# Bloqueia in-source build
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR)
    message(FATAL_ERROR "Build in-source nao permitido. Crie uma pasta build/:\n  mkdir build && cd build && cmake ..")
endif()

project(VisualizadorCenas)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

include(FetchContent)

FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG 3.4
)
FetchContent_Declare(
    glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG master
)
FetchContent_Declare(
    stb_image
    GIT_REPOSITORY https://github.com/nothings/stb.git
    GIT_TAG master
)
FetchContent_MakeAvailable(glfw glm stb_image)

include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${CMAKE_SOURCE_DIR}/include/glad)
include_directories(${glm_SOURCE_DIR})
include_directories(${stb_image_SOURCE_DIR})

if(NOT MSVC)
    add_compile_options(-Wno-pragmas)
endif()

if(WIN32)
    set(OPENGL_LIBS opengl32)
elseif(APPLE)
    find_library(OpenGL_LIBRARY OpenGL)
    set(OPENGL_LIBS ${OpenGL_LIBRARY})
else()
    find_package(OpenGL REQUIRED)
    set(OPENGL_LIBS ${OPENGL_gl_LIBRARY})
endif()

set(GLAD_C "${CMAKE_SOURCE_DIR}/common/glad.c")
if(NOT EXISTS ${GLAD_C})
    message(FATAL_ERROR "glad.c nao encontrado em common/. Veja o README.")
endif()

add_executable(VisualizadorCenas
    src/main.cpp
    src/rendering/Texture2D.cpp
    src/rendering/Shader.cpp
    src/scene/Trajectory.cpp
    src/scene/SceneObject.cpp
    src/scene/SceneParser.cpp
    src/loaders/OBJLoader.cpp
    src/input/InputHandler.cpp
    ${GLAD_C}
)
target_include_directories(VisualizadorCenas PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/camera
    ${CMAKE_SOURCE_DIR}/src/rendering
    ${CMAKE_SOURCE_DIR}/src/scene
    ${CMAKE_SOURCE_DIR}/src/loaders
    ${CMAKE_SOURCE_DIR}/src/input
    ${CMAKE_SOURCE_DIR}/include/glad
    ${glm_SOURCE_DIR}
    ${stb_image_SOURCE_DIR}
)
target_link_libraries(VisualizadorCenas glfw ${OPENGL_LIBS})
