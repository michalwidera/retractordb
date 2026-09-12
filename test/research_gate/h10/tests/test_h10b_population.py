#!/usr/bin/env python3
"""Populacja H10b jest węzłowa, również pod korzeniem i przy wielu `#`."""

import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "oracle"))
sys.path.insert(0, str(ROOT))

from plan import Plan, make_hash, make_pass, make_source  # noqa: E402
from run_campaign import eligible_h10b, h10b_form  # noqa: E402


def main():
    left = make_source("left", 1, 1)
    right = make_source("right", 4, 1)
    direct = make_hash("direct", left, right)
    reverse = make_hash("reverse", right, left)
    computed = make_hash("computed", direct, right)
    root = make_pass("root", computed)
    plan = Plan((left, right, direct, reverse, computed, root))

    assert plan.root == root
    assert [node.name for node in plan.nodes if eligible_h10b(plan, node)] == [
        "direct", "reverse"]
    assert h10b_form(plan, direct) == 4
    assert h10b_form(plan, reverse) == 1
    print("H10b: oba niekorzeniowe `#` z deklaracjami wybrane; `#` obliczany odrzucony")


if __name__ == "__main__":
    main()
