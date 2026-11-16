## Summary

<!-- What changed and why. Link the issue if one exists. -->

## Type of change

- [ ] Bug fix
- [ ] New example / feature
- [ ] Engine refactor
- [ ] Build, CI or tooling
- [ ] Documentation

## Checklist

- [ ] Debug build passes with `-DENGINE_WARNINGS_AS_ERRORS=ON`
- [ ] `ctest --test-dir build --output-on-failure` passes
- [ ] New non-GL logic has unit tests
- [ ] No GL call bypasses `GLCall`
- [ ] README tables and `CHANGELOG.md` are updated when behaviour changes
- [ ] Both `README.md` and `README.zh-CN.md` are updated for user-facing docs
- [ ] No credentials, tokens or local paths are committed

## Testing

<!-- What did you run, and on which platform / GPU? -->

---

## 中文摘要

<!-- 可选：用一两句话说明改动。 -->

- [ ] Debug 构建在 `-Werror` 下通过，`ctest` 全部通过
- [ ] 面向用户的文档中英文版本已同步
- [ ] 没有提交任何凭据、令牌或本地绝对路径
