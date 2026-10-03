#!/usr/bin/env python3
# PreToolUse (Bash): ctest i ninja test* zwracaja tylko ogon wyjscia, calosc idzie do logu.
# Powod: pelny log testow wczytany do sesji wraca w kazdej kolejnej turze (CLAUDE.md,
# "Delegation to subagents"). Przepisujemy tylko proste polecenia zaczynajace sie od ctest
# albo ninja test / test-valgrind / test_gate: bez potokow, przekierowan, srednikow,
# podpowloki i bez listowania (-N, --show-only), ktore ma byc widoczne w calosci.
# Wejscie: JSON na stdin. Wyjscie: {} (bez decyzji) albo updatedInput z decyzja allow -
# te same polecenia sa na liscie permissions.allow w .claude/settings.json.
import json
import os
import re
import shlex
import sys
import time

TAIL_LINES = 120
RUN_RE = re.compile(r"^(ctest|ninja\s+test(?:-valgrind|_gate)?)(\s|$)")
UNSAFE_RE = re.compile(r"[|&;<>`$()\n]")
LIST_RE = re.compile(r"(^|\s)(-N|--show-only(=\S+)?|--print-labels|--help|-h|--version)(\s|$)")


def no_decision() -> int:
    print("{}")
    return 0


def main() -> int:
    try:
        data = json.load(sys.stdin)
    except Exception:
        return no_decision()
    if data.get("tool_name") != "Bash":
        return no_decision()
    tool_input = data.get("tool_input") or {}
    command = (tool_input.get("command") or "").strip()
    if not RUN_RE.match(command) or UNSAFE_RE.search(command) or LIST_RE.search(command):
        return no_decision()
    base = data.get("scratchpad_dir") or os.environ.get("TMPDIR") or "/tmp"
    log_dir = os.path.join(base, "test-logs")
    try:
        os.makedirs(log_dir, exist_ok=True)
    except OSError:
        return no_decision()
    stamp = time.strftime("%Y%m%d-%H%M%S")
    name = re.sub(r"[^A-Za-z0-9_.-]+", "_", command)[:40]
    log = shlex.quote(os.path.join(log_dir, f"{stamp}-{name}.log"))
    # Podpowloka: kod wyjscia polecenia (PIPESTATUS[0]) zostaje kodem calosci, a `exit` nie
    # zamyka powloki narzedzia. `tee` zapisuje calosc, `tail` pokazuje koniec, gdzie ctest
    # drukuje podsumowanie i liste nieudanych testow.
    rewritten = (
        f"( {{ {command}; }} 2>&1 | tee {log} | tail -n {TAIL_LINES}; "
        f"__rc=${{PIPESTATUS[0]}}; "
        f"printf '[test-output-filter] exit %s, full output: %s\\n' \"$__rc\" {log}; "
        f'exit "$__rc" )'
    )
    new_input = dict(tool_input)
    new_input["command"] = rewritten
    out = {
        "hookSpecificOutput": {
            "hookEventName": "PreToolUse",
            "permissionDecision": "allow",
            "permissionDecisionReason": f"test-output-filter: tail of {TAIL_LINES} lines, full output in {log}",
            "updatedInput": new_input,
        }
    }
    print(json.dumps(out))
    return 0


if __name__ == "__main__":
    sys.exit(main())
