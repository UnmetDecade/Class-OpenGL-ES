# 架构约定

> 本文件是项目的长期约定，后续阶段不得违反。

## 1. 硬性约束

| 约束 | 说明 |
|---|---|
| 标准 | C++17，OpenGL ES 3.0（GLSL ES 3.00），GLFW3，glad2 |
| 资源管理 | 所有 GL 对象（program/buffer/VAO/texture）一律 RAII |
| 拷贝语义 | 禁用拷贝构造和拷贝赋值，允许移动构造和移动赋值 |
| 错误检查 | 所有 GL 调用必须经 `GLCall` 宏包裹，出错即打印调用位置并中断 |
| 构建 | CMake ≥ 3.16，GLFW/glm 使用仓库内源码，glad2 生成的 GLES2 loader 放入 `third_party/glad/` |
| 目录结构 | 严格按下方结构组织，不要乱放文件 |
| 范围控制 | 每个阶段只做该阶段要求的功能，不要顺手实现额外功能 |

## 3. 各模块接口约定

- `Window`：构造（宽/高/标题/可选 vsync），`ShouldClose()`、`SwapBuffers()`、`PollEvents()`、回调设置（窗口尺寸变化、键盘、鼠标）。
- `Shader`：
  - 构造：`Shader(const std::string& vertPath, const std::string& fragPath)` 与 `Shader(const char* vertSrc, const char* fragSrc)`
  - 编译或链接失败 → 抛 `std::runtime_error`，消息含完整 info log 和失败 stage（VERTEX/FRAGMENT/LINK）
  - `Bind()` / `Unbind()`（即 glUseProgram(0)）
  - `SetUniform(const std::string& name, ...)`：支持 int / float / glm::vec2/3/4 / glm::mat3/4，mat4 需转置参数 `bool transpose = false`
  - 内部缓存 uniform location，避免重复 glGetUniformLocation
- `Buffer`（VBO）：构造即 `glGenBuffers` + `glBufferData`；`SetData(const void* data, size_t size, GLenum usage)`；`Bind/Unbind`；模板方法 `SetLayout` 不放在此类（布局归 VAO 管）。
- `IndexBuffer`（EBO）：同上，`GetCount()`。
- `VertexArray`：`AddVertexBuffer(const VertexBuffer&, 布局描述)`，布局描述用 `struct VertexAttribute{int size; GLenum type; bool normalized; size_t stride; size_t offset;}` 数组，内部完成 glVertexAttribPointer + glEnableVertexAttribArray 并维护 index。
- `Texture`：构造（文件路径，自动判断是否有 alpha 选 GL_RGBA/GL_RGB），stb_image 加载；`Bind(unsigned int slot = 0)`，析构 glDeleteTextures；翻转 Y（stbi_set_flip_vertically_on_load）。
- `Camera`：透视相机，`GetViewMatrix()` / `GetProjectionMatrix(float aspect)` / `ProcessKeyboard(dt, dir)` / `ProcessMouse(dx, dy)`，yaw/pitch 欧拉角实现。
- `Renderer`：静态方法 `Clear(float r, float g, float b, float a)`、`Draw(const VertexArray&, const IndexBuffer&, const Shader&)`、`DrawArrays(...)`。
