cmake_minimum_required(VERSION 3.16)
project(ClassOpenGLES LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
endif()

include(CTest)

option(ENGINE_WARNINGS_AS_ERRORS "Treat engine compiler warnings as errors" OFF)
option(ENGINE_ENABLE_COVERAGE "Instrument unit tests for gcov coverage" OFF)

function(engine_enable_warnings target)
    if(MSVC)
        target_compile_options(${target} PRIVATE /W4)
        if(ENGINE_WARNINGS_AS_ERRORS)
            target_compile_options(${target} PRIVATE /WX)
        endif()
    else()
        target_compile_options(${target} PRIVATE -Wall -Wextra)
        if(ENGINE_WARNINGS_AS_ERRORS)
            target_compile_options(${target} PRIVATE -Werror)
        endif()
    endif()
endfunction()

# ---------------------------------------------------------------------------
# Dependencies: use the vendored copies when present, otherwise fetch them.
# ---------------------------------------------------------------------------
include(FetchContent)

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/glfw/CMakeLists.txt)
    message(STATUS "Using vendored GLFW")
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/glfw ${CMAKE_BINARY_DIR}/glfw)
else()
    FetchContent_Declare(
        glfw
        GIT_REPOSITORY https://github.com/glfw/glfw.git
        GIT_TAG        3.4
        GIT_SHALLOW    TRUE
    )
    FetchContent_MakeAvailable(glfw)
endif()

set(GLM_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLM_BUILD_INSTALL OFF CACHE BOOL "" FORCE)

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/glm/CMakeLists.txt)
    message(STATUS "Using vendored glm")
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/glm ${CMAKE_BINARY_DIR}/glm)
else()
    FetchContent_Declare(
        glm
        GIT_REPOSITORY https://github.com/g-truc/glm.git
        GIT_TAG        1.0.1
        GIT_SHALLOW    TRUE
    )
    FetchContent_MakeAvailable(glm)
endif()

# ---------------------------------------------------------------------------
# glad (pre-generated OpenGL ES 3.0 loader)
# ---------------------------------------------------------------------------
add_library(glad STATIC third_party/glad/src/gles2.c)
target_include_directories(glad PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/third_party/glad/include)

# ---------------------------------------------------------------------------
# engine
# ---------------------------------------------------------------------------
add_library(engine STATIC
    engine/Window/Window.cpp
    engine/Renderer/Shader.cpp
    engine/Renderer/Buffer.cpp
    engine/Renderer/VertexArray.cpp
    engine/Renderer/Texture.cpp
    engine/Renderer/Camera.cpp
    engine/Renderer/Renderer.cpp
    engine/Renderer/ShaderLibrary.cpp
    engine/Renderer/RenderCommand.cpp
    engine/Renderer/OpenGLRendererAPI.cpp
    engine/Renderer/NullRendererAPI.cpp
    engine/Renderer/Mesh.cpp
    engine/Renderer/Model.cpp
    engine/Renderer/ObjLoader.cpp
    engine/Renderer/TextRenderer.cpp
    engine/Renderer/Framebuffer.cpp
    engine/Renderer/PostProcess.cpp
    engine/Renderer/ShadowMap.cpp
    engine/Renderer/Cubemap.cpp
)
target_include_directories(engine PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
# Vendored stb headers define static helpers we do not call; SYSTEM keeps
# their warnings out of the engine's -Wall -Wextra -Werror build.
target_include_directories(engine SYSTEM PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/third_party/stb)
target_link_libraries(engine PUBLIC glad glfw glm::glm)
target_compile_definitions(engine
    PUBLIC $<$<CONFIG:Debug>:DEBUG>
    PUBLIC ASSET_DIR="${CMAKE_CURRENT_SOURCE_DIR}/assets"
)

# ---------------------------------------------------------------------------
# examples
# ---------------------------------------------------------------------------
add_executable(01_triangle engine/examples/01_triangle/main.cpp)
target_link_libraries(01_triangle PRIVATE engine)

add_executable(02_texture engine/examples/02_texture/main.cpp)
target_link_libraries(02_texture PRIVATE engine)

add_executable(03_cube engine/examples/03_cube/main.cpp)
target_link_libraries(03_cube PRIVATE engine)

add_executable(04_shader_library engine/examples/04_shader_library/main.cpp)
target_link_libraries(04_shader_library PRIVATE engine)

add_executable(05_lighting engine/examples/05_lighting/main.cpp)
target_link_libraries(05_lighting PRIVATE engine)

add_executable(06_text engine/examples/06_text/main.cpp)
target_link_libraries(06_text PRIVATE engine)

add_executable(07_backend_switch engine/examples/07_backend_switch/main.cpp)
target_link_libraries(07_backend_switch PRIVATE engine)

add_executable(08_postprocess engine/examples/08_postprocess/main.cpp)
target_link_libraries(08_postprocess PRIVATE engine)

add_executable(09_shadow engine/examples/09_shadow/main.cpp)
target_link_libraries(09_shadow PRIVATE engine)

add_executable(10_skybox engine/examples/10_skybox/main.cpp)
target_link_libraries(10_skybox PRIVATE engine)

add_executable(11_hdr engine/examples/11_hdr/main.cpp)
target_link_libraries(11_hdr PRIVATE engine)

add_executable(12_pbr engine/examples/12_pbr/main.cpp)
target_link_libraries(12_pbr PRIVATE engine)

add_executable(13_rose engine/examples/13_rose/main.cpp)
target_link_libraries(13_rose PRIVATE engine)

# ---------------------------------------------------------------------------
# warnings
# ---------------------------------------------------------------------------
engine_enable_warnings(engine)

foreach(example
    01_triangle 02_texture 03_cube 04_shader_library 05_lighting 06_text
    07_backend_switch 08_postprocess 09_shadow 10_skybox 11_hdr 12_pbr 13_rose)
    engine_enable_warnings(${example})
endforeach()

# ---------------------------------------------------------------------------
# tests (headless: no OpenGL ES context required)
# ---------------------------------------------------------------------------
if(BUILD_TESTING)
    add_executable(camera_tests tests/CameraTests.cpp engine/Renderer/Camera.cpp)
    target_include_directories(camera_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    target_link_libraries(camera_tests PRIVATE glm::glm)
    engine_enable_warnings(camera_tests)
    add_test(NAME camera_tests COMMAND camera_tests)

    add_executable(obj_loader_tests tests/ObjLoaderTests.cpp engine/Renderer/ObjLoader.cpp)
    target_include_directories(obj_loader_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    target_link_libraries(obj_loader_tests PRIVATE glm::glm)
    engine_enable_warnings(obj_loader_tests)
    add_test(NAME obj_loader_tests COMMAND obj_loader_tests)

    if(ENGINE_ENABLE_COVERAGE)
        foreach(test_target camera_tests obj_loader_tests)
            target_compile_options(${test_target} PRIVATE --coverage -O0 -g)
            target_link_options(${test_target} PRIVATE --coverage)
        endforeach()
    endif()
endif()
