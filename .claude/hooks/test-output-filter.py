#!/usr/bin/env python3
# PreToolUse (Bash): ctest i ninja test* zwracaja tylko ogon wyjscia, calosc idzie do logu.
# Powod: pelny log testow wczytany do sesji wraca w kazdej kolejnej turze (CLAUDE.md,
# "Delegation to subagents"). Przepisujemy tylko proste polecenia: `ctest ...` albo
# `ninja [-C <katalog>] test|test-valgrind|test_gate`, oba opcjonalnie po `cd <katalog> &&`.
# Poza cudzyslowem nie moze byc innego operatora (potok, przekierowanie, srednik, podpowloka),
# a $, ` i \ tylko w apostrofach; listowanie (-N, --show-only) ma byc widoczne w calosci.
# Niezaleznie od formy polecenia hook odmawia trybow skryptowych ctest (-S, -D, --script,
# --build-and-test, --dashboard): wykonuja dowolny kod, a `Bash(ctest *)` z listy
# permissions.allow w .claude/settings.json by je przepuscilo.
# Wejscie: JSON na stdin. Wyjscie: {} (bez decyzji), deny albo updatedInput z decyzja allow -
# te same polecenia sa na liscie permissions.allow w .claude/settings.json.
import json
import os
import re
import shlex
import sys
import time

TAIL_LINES = 120
OPS = "();<>|&\n"
NINJA_TARGETS = {"test", "test-valgrind", "test_gate"}
LIST_ARGS = {"-N", "--print-labels", "--help", "-h", "--version"}


def no_decision() -> int:
    print("{}")
    return 0


def decide(decision: str, reason: str, new_input=None) -> int:
    out = {"hookEventName": "PreToolUse", "permissionDecision": decision, "permissionDecisionReason": reason}
    if new_input is not None:
        out["updatedInput"] = new_input
    print(json.dumps({"hookSpecificOutput": out}))
    return 0


def lex(command: str):
    """Tokeny jak w powloce: lista (tekst, czy_operator) z cudzyslowami zdjetymi; operator liczy sie
    tylko poza cudzyslowem. Drugi wynik: czy poza apostrofami stoi $, ` albo \\. None, gdy
    cudzyslow nie jest domkniety."""
    tokens, word, in_word, quote, risky, i = [], [], False, None, False, 0

    def flush():
        nonlocal word, in_word
        if in_word:
            tokens.append(("".join(word), False))
        word, in_word = [], False

    while i < len(command):
        c = command[i]
        if quote == "'":
            if c == "'":
                quote = None
            else:
                word.append(c)
        elif c == "\\" and i + 1 < len(command) and (quote is None or command[i + 1] in '"\\$`'):
            risky = True
            word.append(command[i + 1])
            in_word = True
            i += 2
            continue
        elif quote == '"':
            if c == '"':
                quote = None
            else:
                risky = risky or c in "$`\\"
                word.append(c)
        elif c in "'\"":
            quote, in_word = c, True
        elif c in OPS:
            flush()
            j = i
            while j < len(command) and command[j] in OPS:
                j += 1
            tokens.append((command[i:j], True))
            i = j
            continue
        elif c.isspace():
            flush()
        else:
            risky = risky or c in "$`\\"
            word.append(c)
            in_word = True
        i += 1
    if quote:
        return None
    flush()
    return tokens, risky


def script_arg(tokens):
    """Argument trybu skryptowego w dowolnym wywolaniu ctest w poleceniu, albo None."""
    in_ctest = False
    for text, is_op in tokens:
        if is_op:
            in_ctest = False
        elif text == "ctest" or text.endswith("/ctest"):
            in_ctest = True
        elif in_ctest and (text in ("--build-and-test", "--dashboard") or text.startswith(("-S", "-D", "--script"))):
            return text
    return None


def filterable(tokens) -> bool:
    ops = [i for i, (_, is_op) in enumerate(tokens) if is_op]
    if ops:
        # Jedyny dopuszczalny operator: `cd <katalog> &&` na poczatku.
        if len(ops) != 1 or tokens[ops[0]][0] != "&&" or ops[0] != 2 or tokens[0][0] != "cd":
            return False
        tokens = tokens[3:]
    words = [text for text, _ in tokens]
    if words[:1] == ["ctest"]:
        return not any(w in LIST_ARGS or w.startswith("--show-only") for w in words[1:])
    if words[:1] == ["ninja"]:
        rest = words[1:]
        if rest[:1] == ["-C"]:
            rest = rest[2:]
        return len(rest) == 1 and rest[0] in NINJA_TARGETS
    return False


def main() -> int:
    try:
        data = json.load(sys.stdin)
    except Exception:
        return no_decision()
    if data.get("tool_name") != "Bash":
        return no_decision()
    tool_input = data.get("tool_input") or {}
    command = (tool_input.get("command") or "").strip()
    lexed = lex(command)
    if lexed is None:
        return no_decision()
    tokens, risky = lexed
    arg = script_arg(tokens)
    if arg:
        return decide("deny", f"test-output-filter: `ctest {arg}` runs a script or dashboard, i.e. arbitrary code; ask the human")
    if risky or not filterable(tokens):
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
    # zamyka powloki narzedzia; `cd` z polecenia tez zostaje w podpowloce. `tee` zapisuje calosc,
    # `tail` pokazuje koniec, gdzie ctest drukuje podsumowanie i liste nieudanych testow.
    rewritten = (
        f"( {{ {command}; }} 2>&1 | tee {log} | tail -n {TAIL_LINES}; "
        f"__rc=${{PIPESTATUS[0]}}; "
        f"printf '[test-output-filter] exit %s, full output: %s\\n' \"$__rc\" {log}; "
        f'exit "$__rc" )'
    )
    new_input = dict(tool_input)
    new_input["command"] = rewritten
    return decide("allow", f"test-output-filter: tail of {TAIL_LINES} lines, full output in {log}", new_input)


if __name__ == "__main__":
    sys.exit(main())
