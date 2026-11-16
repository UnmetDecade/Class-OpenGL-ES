# Changelog

<div align="left"><b>English</b> · <a href="CHANGELOG.zh-CN.md">简体中文</a></div>

All notable changes to this project are documented here. The format follows
[Keep a Changelog](https://keepachangelog.com/en/1.1.0/) and the project uses
[Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added

- Repository standards: contribution guide, security policy, code of conduct,
  issue forms, pull request template and editor configuration.
- GitHub Actions CI: Linux and macOS builds with `-Wall -Wextra -Werror`,
  headless unit tests, a coverage gate and a tag-only release job.
- CodeQL analysis for C++ and Dependabot updates for GitHub Actions.
- Headless unit tests for `Camera` and the Wavefront OBJ parser, run through
  CTest without a GL context.
- `scripts/check_coverage.py`, enforcing a global coverage floor plus stricter
  per-file floors for `Camera.cpp` and `ObjLoader.cpp`.
- Bilingual GitHub Pages landing page under `docs/`.

### Changed

- OBJ parsing moved from `Model` into `ObjLoader`, so the import logic is
  testable without OpenGL ES. `Vertex` now lives in its own header.
- `README.md` is the English default, with `README.zh-CN.md` keeping the
  Chinese original.
- `ARCHITECTURE.md` is now English; the Chinese version moved to
  `ARCHITECTURE.zh-CN.md`.
- The engine and examples are built with `-Wall -Wextra`, optionally promoted
  to errors with `-DENGINE_WARNINGS_AS_ERRORS=ON`.
- Vendored stb headers are included as system headers so their static helpers
  do not fail the strict warning build.
- Verification notes now describe the Ubuntu 22.04 / Mesa llvmpipe environment
  that actually runs this ES port; stale desktop-only macOS claims were removed.

### Fixed

- `EffectEntry::Shader` member renamed to `EffectEntry::Program` to resolve a
  GCC "changes meaning of 'Shader'" error.

### Notes

- No release tags exist yet. Once `v0.1.0` is tagged, entries move out of
  `[Unreleased]` and gain compare links.

[Unreleased]: https://github.com/UnmetDecade/Class-OpenGL-ES/commits/main
