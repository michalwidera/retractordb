#!/usr/bin/env bash
# Generuje docker/ci/DockerConan.txt na podstawie conanfile.py.
# Uruchamiać z katalogu głównego repo, scripts/ lub build/Debug/.

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/../.." && pwd)"
CONANFILE="${1:-${REPO_ROOT}/conanfile.py}"
OUTPUT="${2:-${REPO_ROOT}/docker/ci/DockerConan.txt}"

if [[ ! -f "${CONANFILE}" ]]; then
  echo "Błąd: nie znaleziono ${CONANFILE}" >&2
  exit 1
fi

python3 - "${CONANFILE}" "${OUTPUT}" << 'PYEOF'
import ast, sys
from datetime import datetime, timezone

conanfile_path = sys.argv[1]
output_path    = sys.argv[2]

with open(conanfile_path) as f:
    tree = ast.parse(f.read())

requires   = []
generators = []
options    = {}

for node in ast.walk(tree):
    if not isinstance(node, ast.ClassDef):
        continue
    for item in node.body:
        if not isinstance(item, ast.Assign):
            continue
        for target in item.targets:
            if not isinstance(target, ast.Name):
                continue
            name = target.id
            val  = item.value
            if name == "requires" and isinstance(val, ast.Tuple):
                for elt in val.elts:
                    if isinstance(elt, ast.Constant):
                        requires.append(elt.value)
            elif name == "generators" and isinstance(val, ast.Tuple):
                for elt in val.elts:
                    if isinstance(elt, ast.Constant):
                        generators.append(elt.value)
            elif name == "default_options" and isinstance(val, ast.Dict):
                for k, v in zip(val.keys, val.values):
                    if isinstance(k, ast.Constant) and isinstance(v, ast.Constant):
                        options[k.value] = v.value

timestamp = datetime.now(timezone.utc).strftime("%Y-%m-%d %H:%M UTC")

lines = [
    f"# Wygenerowano automatycznie przez scripts/gen_dockerconan.sh ({timestamp}).",
    "# Nie modyfikować ręcznie — zmiany wprowadzać w conanfile.py.",
    "",
    "[requires]",
    *requires,
    "[options]",
    *[f"{k}={str(v)}" for k, v in options.items()],
    "[generators]",
    *generators,
    "[layout]",
    "cmake_layout",
]

with open(output_path, "w") as f:
    f.write("\n".join(lines) + "\n")

print(f"Wygenerowano {output_path}")
PYEOF
