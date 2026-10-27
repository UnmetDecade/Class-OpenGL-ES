cmake_minimum_required(VERSION 3.16)
project(ClassOpenGLES LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
endif()

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
    engine/Renderer/TextRenderer.cpp
    engine/Renderer/Framebuffer.cpp
    engine/Renderer/PostProcess.cpp
    engine/Renderer/ShadowMap.cpp
    engine/Renderer/Cubemap.cpp
)
target_include_directories(engine
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/third_party/stb
)
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
