# 贡献指南

<div align="left"><a href="CONTRIBUTING.md">English</a> · <b>简体中文</b></div>

感谢你愿意参与贡献。本项目是面向教学的 OpenGL ES 引擎：代码小而直白、可读性优先，
不追求复杂的抽象。

## 开始之前

- 新示例或较大的引擎改动请先开 issue，等范围达成一致后再写代码。
- 一个示例只讲一种能力，不要为了新示例去重写旧示例。
- 先读 [ARCHITECTURE.zh-CN.md](ARCHITECTURE.zh-CN.md)，其中的约定不可违反。

## 环境准备

- CMake ≥ 3.16 与支持 C++17 的编译器。
- Linux（示例的运行平台）：需要 GLFW、EGL 与 GLES 头文件。

  ```bash
  sudo apt-get install build-essential cmake \
      xorg-dev libgl1-mesa-dev libegl1-mesa-dev libgles2-mesa-dev
  ```

- macOS 可以构建项目并运行无窗口单元测试，但 GLFW 在 macOS 上无法创建 OpenGL ES
  上下文，示例需要带 EGL 的 Linux 环境。
- 当 `glfw/`、`glm/` 不存在时由 CMake 自动拉取；本地副本优先，用于离线构建。

## 构建与测试

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DENGINE_WARNINGS_AS_ERRORS=ON
cmake --build build -j
ctest --test-dir build --output-on-failure
```

`tests/` 下的单元测试必须保持无窗口：不创建 GL 上下文，不依赖仓库以外的资源文件。

## 代码约定

- C++17，4 空格缩进，大括号独占一行。
- 所有 GL 资源一律 RAII：禁止拷贝、默认移动、析构释放。
- 所有 GL 调用必须经过 `GLCall`，不要直接调用 GL。
- 注释和日志输出一律使用英文，包括错误字符串和 `LOG_*` 消息。
- 公共头文件描述接口契约，不描述实现细节。
- 解析与数学代码不要引入 GL 头文件，保持可单元测试。

## 测试

- 新增纯逻辑（解析、数学、顺序控制）时，在 `tests/` 下补测试。
- 测试使用 `tests/TestFramework.h` 里的小型 runner，不引入外部框架。
- 不要为了让改动通过而降低 `scripts/check_coverage.py` 的门槛，应该补测试。

## 提交与 PR

- 提交信息使用简短的祈使句，可选前缀：`feat:`、`fix:`、`docs:`、`test:`、
  `build:`、`ci:`。
- 一次提交只做一件事，不要混入纯格式化改动。
- 开 PR 前先用 `-DENGINE_WARNINGS_AS_ERRORS=ON` 构建 Debug，并跑完整测试。
- 面向用户的改动需要同步更新 `CHANGELOG.md` 与 `CHANGELOG.zh-CN.md`。
- 用户文档的中英文版本需要同步修改。
- 不要提交凭据、令牌、本地绝对路径或构建产物。

## 问题反馈

- Bug 与功能建议：使用 issue 表单。
- 安全漏洞：按 [SECURITY.zh-CN.md](SECURITY.zh-CN.md) 私下上报，不要开公开 issue。
