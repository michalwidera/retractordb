#!/usr/bin/env bash
# Proba dymna J5 (docs/jupyter-integration.md, sekcja 10): notatnik
# api/python/notebooks/colab_quickstart.ipynb wykonany przeciw zbudowanemu kolu, w
# czystym Pythonie bez narzedzi budowy i z tym, co Colab ma zainstalowane fabrycznie.
# Sprawdza trzy rzeczy, ktorych api/python/tests nie widzi:
#   1. kolo instaluje sie tak, jak zrobi to uzytkownik Colab (%pip install w samym
#      notatniku), a kazda komorka notatnika przechodzi,
#   2. instalacja niczego poza dodaniem retractordb nie zmienia (pip freeze --all
#      przed i po) - to jest warunek "bez restartu runtime" z sekcji 4,
#   3. komorka instalacji z importem miesci sie w 20 s (kryterium z sekcji 4).
#
# Srodowisko jak Colab runtime 2026.07 (research.google.com/colaboratory/
# runtime-version-faq.html): Python 3.12, numpy 2.0.2, torch 2.11.0. Przy zmianie
# runtime Colab wersje ponizej podnosi sie razem z tym komentarzem.
#
# Uzycie - w czystym kontenerze z Pythonem 3.12, korzen repo tylko do odczytu:
#   docker run --rm -v "$PWD:/src:ro" python:3.12-slim bash /src/docker/wheel/notebook-smoke.sh
# albo na dowolnym Linuksie z python3.12 (PYTHON wskazuje inny interpreter):
#   docker/wheel/notebook-smoke.sh [kolo.whl]
# Bez argumentu bierze jedyne kolo cp312-abi3 dla architektury hosta z wheelhouse/
# (docker/wheel/build-wheels.sh).
#
# torch domyslnie z indeksu CPU PyTorch: okolo 200 MB zamiast kilku GB bibliotek CUDA,
# ktorych proba i tak nie uzywa. RDB_TORCH_INDEX= (pusta wartosc) bierze go z PyPI -
# na x86_64 to wariant z CUDA, jak w samym Colabie, i jedyna droga tam, gdzie
# download.pytorch.org jest niedostepny.
set -euo pipefail

readonly numpy_version=2.0.2
readonly torch_version=2.11.0
readonly python=${PYTHON:-python3.12}
readonly torch_index=${RDB_TORCH_INDEX-https://download.pytorch.org/whl/cpu}

repo_dir=$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)
readonly notebook=$repo_dir/api/python/notebooks/colab_quickstart.ipynb

# Kola sa wylacznie linuksowe (manylinux): na macOS pip odrzucilby je dopiero po
# kilku minutach stawiania srodowiska, a `uname -m` podaje tam arm64 zamiast aarch64,
# wiec nie znalazlby nawet pliku. Tam proba biegnie w kontenerze - na Apple silicon
# linux/arm64, czyli na kole aarch64.
if [[ $(uname -s) != Linux ]]; then
  echo "notebook-smoke: the wheels are Linux-only - on $(uname -s) run this in a Linux container:" >&2
  echo "  docker run --rm -v '$repo_dir:/src:ro' python:3.12-slim bash /src/docker/wheel/notebook-smoke.sh" >&2
  exit 2
fi

case $# in
  0)
    shopt -s nullglob
    wheels=("$repo_dir"/wheelhouse/retractordb-*-cp312-abi3-*_"$(uname -m)".whl)
    if [[ ${#wheels[@]} -ne 1 ]]; then
      echo "notebook-smoke: expected one cp312-abi3 $(uname -m) wheel in $repo_dir/wheelhouse, found ${#wheels[@]} - pass the wheel as an argument" >&2
      exit 2
    fi
    wheel=${wheels[0]}
    ;;
  1) wheel=$1 ;;
  *)
    echo "usage: $0 [wheel]" >&2
    exit 2
    ;;
esac
[[ -f $wheel ]] || {
  echo "notebook-smoke: no such wheel: $wheel" >&2
  exit 2
}
# Sciezka bezwzgledna: jadro notatnika startuje w innym katalogu roboczym.
wheel=$(cd "$(dirname "$wheel")" && pwd)/$(basename "$wheel")

"$python" -c 'import sys; sys.exit(sys.version_info[:2] != (3, 12))' || {
  echo "notebook-smoke: $python is not Python 3.12, which Colab runs - set PYTHON" >&2
  exit 2
}

work_dir=$(mktemp -d "${TMPDIR:-/tmp}/rdb-notebook-smoke.XXXXXX")
trap 'rm -rf -- "$work_dir"' EXIT
venv_python=$work_dir/venv/bin/python3

# Stan "fabryczny" Colab: numpy i torch w wersjach runtime oraz jadro Jupytera, ktore
# Colab tez ma. nbclient wykonuje notatnik; w Colabie jego role gra sam frontend.
"$python" -m venv "$work_dir/venv"
"$venv_python" -m pip install --quiet --disable-pip-version-check "numpy==$numpy_version" nbclient ipykernel
torch_args=()
if [[ -n $torch_index ]]; then torch_args=(--index-url "$torch_index"); fi
"$venv_python" -m pip install --quiet --disable-pip-version-check "${torch_args[@]}" "torch==$torch_version"

echo "notebook-smoke: $(basename "$wheel") on $("$venv_python" --version), numpy $numpy_version, torch $torch_version"

# JUPYTER_DATA_DIR: jadro "python3" ma byc tym z venv, a nie specyfikacja jadra
# zostawiona w katalogu uzytkownika hosta. RDB_WHEEL czyta komorka instalacji.
JUPYTER_DATA_DIR=$work_dir/jupyter RDB_WHEEL=$wheel "$venv_python" - "$notebook" "$work_dir" << 'PY'
import re
import subprocess
import sys
from datetime import datetime
from pathlib import Path

import nbformat
from nbclient import NotebookClient

INSTALL_BUDGET_S = 20.0

notebook, work_dir = Path(sys.argv[1]), Path(sys.argv[2])


def freeze() -> set[str]:
    listing = subprocess.run(
        [sys.executable, "-m", "pip", "freeze", "--all", "--disable-pip-version-check"],
        check=True, capture_output=True, text=True,
    ).stdout
    return set(listing.splitlines())


def seconds(cell) -> float:
    timing = cell.metadata["execution"]
    start, end = (datetime.fromisoformat(timing[k]) for k in ("iopub.execute_input", "shell.execute_reply"))
    return (end - start).total_seconds()


before = freeze()
nb = nbformat.read(notebook, as_version=4)
# Blad komorki konczy skrypt wyjatkiem CellExecutionError z traceback z jadra.
NotebookClient(nb, timeout=600, kernel_name="python3", resources={"metadata": {"path": str(work_dir)}}).execute()
after = freeze()

for cell in nb.cells:
    if cell.cell_type != "code":
        continue
    print(f"--- {cell.id}: {seconds(cell):.1f} s")
    for output in cell.outputs:
        if output.output_type == "stream":
            print(output.text, end="")

failures = []
added = sorted(after - before)
removed = sorted(before - after)
if removed or [re.split(r"[ =@]", line, maxsplit=1)[0].lower() for line in added] != ["retractordb"]:
    failures.append(
        "pip changed more than adding retractordb - Colab would ask for a runtime restart:\n"
        + "".join(f"  - {line}\n" for line in removed)
        + "".join(f"  + {line}\n" for line in added)
    )

install = next(cell for cell in nb.cells if "install" in cell.metadata.get("tags", []))
if seconds(install) > INSTALL_BUDGET_S:
    failures.append(f"install + import took {seconds(install):.1f} s, over the {INSTALL_BUDGET_S:g} s budget")

if failures:
    sys.exit("notebook-smoke: FAILED\n" + "\n".join(failures))
print(f"notebook-smoke: OK - install + import {seconds(install):.1f} s, pip added only retractordb")
PY
