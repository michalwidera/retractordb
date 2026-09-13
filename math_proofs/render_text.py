#!/usr/bin/env python3
"""Convert VersoManual HTML to readable plain text with intact Lean blocks."""

import html
from html.parser import HTMLParser
from pathlib import Path
import re
import subprocess
import sys


class CodeText(HTMLParser):
    def __init__(self):
        super().__init__(convert_charrefs=True)
        self.parts = []

    def handle_data(self, data):
        self.parts.append(data)


def render(source, destination):
    page = Path(source).read_text(encoding="utf-8")
    code_pattern = re.compile(r'<code class="hl lean block"[^>]*>.*?</code>', re.DOTALL)

    def replace_code(match):
        parser = CodeText()
        parser.feed(match.group())
        return '<pre><code class="language-lean">' + html.escape("".join(parser.parts)) + '</code></pre>'

    page = re.sub(r'<nav class="prev-next-buttons">.*?</nav>', '', page, flags=re.DOTALL)
    page = re.sub(r'<span class="permalink-widget[^>]*>.*?</span>', '', page, flags=re.DOTALL)
    page = code_pattern.sub(replace_code, page)
    result = subprocess.run(
        ["pandoc", "-f", "html", "-t", "plain", "--wrap=none"],
        input=page,
        text=True,
        check=True,
        capture_output=True,
    )
    Path(destination).write_text(result.stdout, encoding="utf-8")


if __name__ == "__main__":
    render(sys.argv[1], sys.argv[2])
