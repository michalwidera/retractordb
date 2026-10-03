#!/usr/bin/env bash
# SessionStart: sesja na modelu ponizej Opusa nie pisze planu (CLAUDE.md, "Who plans").
# Sama regula tekstowa nie wystarczyla - Haiku napisal plan mimo niej (2026-10-03).
# Wejscie: JSON na stdin. Pole "model" bywa pominiete (np. w `claude -p` i po /clear) -
# wtedy regula idzie w postaci warunkowej, a model rozstrzyga ja wobec nazwy ze swojego srodowiska.
input=$(cat)
model=$(printf '%s' "$input" | grep -oE '"model"[[:space:]]*:[[:space:]]*"[^"]*"' | head -n 1 |
  sed -E 's/.*"([^"]*)"$/\1/')
rule="never writes a goal declaration, a plan, implementation steps or a design decision. If the request needs any of these, do not research or draft it: reply in the user's language that planning needs Opus, ask the human to switch with /model opus, and stop. Executing an already approved plan, searching and running checks stay allowed."
case "$model" in
*haiku* | *sonnet*) msg="This session runs on ${model}. Under *Who plans* in CLAUDE.md a session below Opus ${rule}" ;;
"") msg="Check the model named in your environment details now. If it is a Haiku or Sonnet model, then under *Who plans* in CLAUDE.md this session ${rule}" ;;
*) exit 0 ;;
esac
printf '{"hookSpecificOutput":{"hookEventName":"SessionStart","additionalContext":"%s"}}\n' "$msg"
