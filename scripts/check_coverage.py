#!/usr/bin/env python3
"""Fail CI when coverage drops below the repository thresholds.

Reads the JSON produced by `gcovr --json` and checks a global floor plus
stricter per-file floors for the modules every example depends on.
"""

from __future__ import annotations

import json
import sys
from pathlib import Path

GLOBAL_THRESHOLDS = {
    "lines": 95.0,
    "functions": 95.0,
    "branches": 60.0,
}

PER_FILE_THRESHOLDS = {
    "engine/Renderer/Camera.cpp": {
        "lines": 95.0,
        "functions": 100.0,
        # GCC counts more branches than Clang here (60.9% vs 85.7%), so the
        # floor stays compiler-independent while lines/functions stay strict.
        "branches": 55.0,
    },
    "engine/Renderer/ObjLoader.cpp": {
        "lines": 95.0,
        "functions": 100.0,
        "branches": 55.0,
    },
}


def normalise(path: str) -> str:
    return path.replace("\\", "/").lstrip("./")


def counts(entry: dict) -> dict[str, tuple[int, int]]:
    lines = [line for line in entry.get("lines", []) if line.get("count") is not None]
    branches = [branch for line in lines for branch in line.get("branches", [])]
    functions = entry.get("functions", [])

    return {
        "lines": (sum(1 for line in lines if line["count"] > 0), len(lines)),
        "functions": (
            sum(1 for function in functions if function["execution_count"] > 0),
            len(functions),
        ),
        "branches": (sum(1 for branch in branches if branch["count"] > 0), len(branches)),
    }


def percentages(values: dict[str, tuple[int, int]]) -> dict[str, float]:
    return {
        name: 100.0 if total == 0 else 100.0 * hit / total
        for name, (hit, total) in values.items()
    }


def find_entry(files: list[dict], expected: str) -> dict | None:
    for entry in files:
        if normalise(entry["file"]).endswith(expected):
            return entry
    return None


def check(label: str, values: dict[str, float], thresholds: dict[str, float]) -> bool:
    ok = True
    parts = []
    for name, floor in thresholds.items():
        value = values[name]
        status = "ok" if value >= floor else "FAIL"
        ok = ok and value >= floor
        parts.append(f"{name} {value:5.1f}% >= {floor:5.1f}% {status}")
    print(f"{label}: " + " | ".join(parts))
    return ok


def main() -> int:
    if len(sys.argv) != 2:
        print("usage: check_coverage.py <gcovr-json>", file=sys.stderr)
        return 2

    report = json.loads(Path(sys.argv[1]).read_text(encoding="utf-8"))
    files = report.get("files", [])
    if not files:
        print("coverage report contains no files", file=sys.stderr)
        return 1

    aggregate = {"lines": (0, 0), "functions": (0, 0), "branches": (0, 0)}
    for entry in files:
        counts_for_file = counts(entry)
        values = percentages(counts_for_file)
        for key in aggregate:
            hit, total = counts_for_file[key]
            aggregate[key] = (aggregate[key][0] + hit, aggregate[key][1] + total)
        print(f"{normalise(entry['file'])}: lines {values['lines']:.1f}% "
              f"functions {values['functions']:.1f}% "
              f"branches {values['branches']:.1f}%")

    ok = check("global", percentages(aggregate), GLOBAL_THRESHOLDS)

    for expected, thresholds in PER_FILE_THRESHOLDS.items():
        entry = find_entry(files, expected)
        if entry is None:
            print(f"{expected}: MISSING from coverage report")
            ok = False
            continue
        ok = check(expected, percentages(counts(entry)), thresholds) and ok

    return 0 if ok else 1


if __name__ == "__main__":
    raise SystemExit(main())
