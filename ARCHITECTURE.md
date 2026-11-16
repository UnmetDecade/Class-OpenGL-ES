# Architecture Conventions

<div align="left"><b>English</b> · <a href="ARCHITECTURE.zh-CN.md">简体中文</a></div>

> This file records the project's long-term conventions. Later phases must not violate them.

## 1. Hard constraints

| Constraint | Rule |
|---|---|
| Standard | C++17, OpenGL ES 3.0 (GLSL ES 3.00), GLFW3, glad2 |
| Resource management | Every GL object (program / buffer / VAO / texture) is owned by RAII |
| Copy semantics | Copy construction and copy assignment are deleted; move construction and move assignment are allowed |
| Error checking | Every GL call goes through the `GLCall` macro, which prints the call site and breaks on failure |
| Build | CMake ≥ 3.16, GLFW/glm from the repository or FetchContent, glad2-generated GLES2 loader vendored in `third_party/glad/` |
| Directory layout | Follow the structure below strictly; do not scatter files |
| Scope control | Each phase implements only what that phase requires; do not add unrelated features |

## 2. Module interfaces

- `Window`: constructor (width / height / title / optional vsync), `ShouldClose()`, `SwapBuffers()`, `PollEvents()`, and callbacks for window resize, keyboard and mouse.
- `Shader`:
  - Constructors: `Shader(const std::string& vertPath, const std::string& fragPath)` and `Shader(const char* vertSrc, const char* fragSrc)`
  - Compile or link failure throws `std::runtime_error` with the complete info log and the failing stage (VERTEX / FRAGMENT / LINK)
  - `Bind()` / `Unbind()` (that is, `glUseProgram(0)`)
  - `SetUniform(const std::string& name, ...)` supports int / float / glm::vec2/3/4 / glm::mat3/4; mat4 takes a `bool transpose = false` argument
  - Uniform locations are cached internally to avoid repeated `glGetUniformLocation` calls
- `Buffer` (VBO): the constructor calls `glGenBuffers` + `glBufferData`; `SetData(const void* data, size_t size, GLenum usage)`; `Bind/Unbind`. Layout is owned by the VAO, not by this class.
- `IndexBuffer` (EBO): same shape as `Buffer`, plus `GetCount()`.
- `VertexArray`: `AddVertexBuffer(const VertexBuffer&, layout)` where the layout is an array of `struct VertexAttribute{int size; GLenum type; bool normalized; size_t stride; size_t offset;}`. It issues `glVertexAttribPointer` + `glEnableVertexAttribArray` and keeps the attribute index.
- `Texture`: constructor from a file path, choosing `GL_RGBA` / `GL_RGB` automatically based on the alpha channel; loaded with stb_image. `Bind(unsigned int slot = 0)`, `glDeleteTextures` in the destructor, Y flipped via `stbi_set_flip_vertically_on_load`.
- `Camera`: perspective camera with `GetViewMatrix()` / `GetProjectionMatrix(float aspect)` / `ProcessKeyboard(dt, dir)` / `ProcessMouse(dx, dy)`, implemented with yaw/pitch Euler angles.
- `Renderer`: static `Clear(float r, float g, float b, float a)`, `Draw(const VertexArray&, const IndexBuffer&, const Shader&)`, `DrawArrays(...)`.

## 3. Testability

- Parsing and math code must stay free of OpenGL ES headers so it can be unit-tested headless.
- `ObjLoader` owns OBJ parsing and returns plain `Vertex` data; `Model` only uploads that data into GPU meshes.
- `tests/` builds without a window or GL context and runs under CTest.
