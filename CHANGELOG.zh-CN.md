# 更新日志

<div align="left"><a href="CHANGELOG.md">English</a> · <b>简体中文</b></div>

本项目的重要变更都记录在这里。格式遵循
[Keep a Changelog](https://keepachangelog.com/zh-CN/1.1.0/)，版本号遵循
[语义化版本](https://semver.org/lang/zh-CN/)。

## [Unreleased]

### 新增

- 仓库规范化文件：贡献指南、安全策略、行为准则、issue 表单、PR 模板与编辑器配置。
- GitHub Actions CI：Linux / macOS 在 `-Wall -Wextra -Werror` 下构建、无窗口单元测试、
  覆盖率门槛，以及只在 tag 上运行的发布 job。
- C++ CodeQL 分析，以及 GitHub Actions 的 Dependabot 更新。
- `Camera` 与 Wavefront OBJ 解析器的无窗口单元测试，通过 CTest 运行，不需要 GL 上下文。
- `scripts/check_coverage.py`：全局覆盖率门槛，外加 `Camera.cpp`、`ObjLoader.cpp`
  的严格单文件门槛。
- `docs/` 下的中英双语 GitHub Pages 落地页。

### 变更

- OBJ 解析从 `Model` 移入 `ObjLoader`，导入逻辑不再依赖 OpenGL ES，可以做无窗口测试；
  `Vertex` 拆到独立头文件。
- `README.md` 为英文默认版，`README.zh-CN.md` 保留中文原文。
- `ARCHITECTURE.md` 改为英文，中文版移至 `ARCHITECTURE.zh-CN.md`。
- 引擎与示例统一启用 `-Wall -Wextra`，可通过 `-DENGINE_WARNINGS_AS_ERRORS=ON`
  将警告提升为错误。
- 第三方 stb 头文件改为 system include，避免其中的静态辅助函数触发严格告警构建。
- 验证记录改为描述实际运行 ES 移植版的 Ubuntu 22.04 / Mesa llvmpipe 环境，
  删除从桌面版误抄的 macOS 结论。

### 修复

- `EffectEntry::Shader` 成员重命名为 `EffectEntry::Program`，解决 GCC
  "changes meaning of 'Shader'" 编译错误。

### 备注

- 目前还没有发布 tag。打上 `v0.1.0` 后，条目会移出 `[Unreleased]` 并补上 compare 链接。

[Unreleased]: https://github.com/UnmetDecade/Class-OpenGL-ES/commits/main
