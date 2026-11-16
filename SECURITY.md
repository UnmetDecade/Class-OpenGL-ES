# Security Policy

<div align="left"><b>English</b> · <a href="SECURITY.zh-CN.md">简体中文</a></div>

## Supported versions

The project is pre-1.0 and has no release tags yet. Security fixes are applied
to the `main` branch only.

| Version | Supported |
|---|---|
| `main` | ✅ |
| Older commits | ❌ |

## Reporting a vulnerability

Please do **not** open a public issue for a security problem.

Use GitHub's private vulnerability reporting:

1. Open the repository's **Security** tab.
2. Choose **Advisories → Report a vulnerability**.
3. Include the affected example or module, reproduction steps, platform and
   driver, and the impact you expect.

If private reporting is unavailable, contact the repository owner through the
contact method listed on their GitHub profile.

## What to include

- Affected file, example and commit.
- Minimal reproduction: commands, input assets and observed output.
- Whether the issue is memory safety (OBJ parser, buffer uploads), shader
  compilation, asset loading, or build/CI supply chain.
- Any suggested fix, if you have one.

## Scope

In scope:

- Memory-safety bugs in parsers and buffer handling.
- Out-of-bounds reads/writes driven by malformed assets.
- Unsafe shader or asset loading paths.
- CI or dependency configuration that could execute untrusted code.

Out of scope:

- Rendering artefacts caused by a specific EGL/GLES driver or Mesa build.
- Issues that require physical access to the machine.
- Denial of service through intentionally huge assets in a local demo.

## Response

Reports are acknowledged as soon as possible, usually within a few days. This
is a volunteer-maintained teaching project, so no formal SLA is offered.
Please give the maintainers reasonable time to publish a fix before disclosing
the issue publicly.
