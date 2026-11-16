#!/usr/bin/env python3
"""Validate the GitHub Pages landing pages.

Checks that:
  * no anchor element is nested inside another anchor,
  * every relative href/src resolves to a file inside docs/,
  * the English and Chinese pages link to each other.
"""

from __future__ import annotations

import sys
from html.parser import HTMLParser
from pathlib import Path

DOCS = Path(__file__).resolve().parent.parent / "docs"
PAGES = ("index.html", "index.zh-CN.html")


class LinkChecker(HTMLParser):
    def __init__(self) -> None:
        super().__init__(convert_charrefs=True)
        self.depth = 0
        self.nested = 0
        self.links: list[str] = []
        self.page = ""

    def handle_starttag(self, tag: str, attrs: list[tuple[str, str | None]]) -> None:
        if tag == "a":
            if self.depth > 0:
                self.nested += 1
                print(f"{self.page}: nested <a> at line {self.getpos()[0]}")
            self.depth += 1
        attributes = dict(attrs)
        for key in ("href", "src"):
            value = attributes.get(key)
            if value:
                self.links.append(value)

    def handle_endtag(self, tag: str) -> None:
        if tag == "a" and self.depth > 0:
            self.depth -= 1


def check_page(page: str) -> bool:
    path = DOCS / page
    if not path.is_file():
        print(f"missing page: {path}")
        return False

    checker = LinkChecker()
    checker.page = page
    checker.feed(path.read_text(encoding="utf-8"))

    ok = checker.nested == 0
    for link in checker.links:
        if link.startswith(("http://", "https://", "#", "mailto:")):
            continue
        target = (DOCS / link.split("#", 1)[0]).resolve()
        if not target.is_file():
            print(f"{page}: broken relative link {link}")
            ok = False
    return ok


def main() -> int:
    ok = all([check_page(page) for page in PAGES])

    english = (DOCS / "index.html").read_text(encoding="utf-8")
    chinese = (DOCS / "index.zh-CN.html").read_text(encoding="utf-8")
    if "index.zh-CN.html" not in english:
        print("index.html does not link to index.zh-CN.html")
        ok = False
    if "index.html" not in chinese:
        print("index.zh-CN.html does not link to index.html")
        ok = False

    if ok:
        print("docs pages OK")
    return 0 if ok else 1


if __name__ == "__main__":
    raise SystemExit(main())
