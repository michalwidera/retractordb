#!/usr/bin/env bash
# C5/J5: wszystkie trzy notebooki z biezacego drzewa, przeciw kolu albo modulowi
# z build/Debug-Python. Kazdy notebook ma osobne jadro, a jego import _core musi
# wskazywac modul wybrany przez ten skrypt. Wyjscia pozostaja poza repozytorium.
#
# Kolo, w czystym kontenerze (repo tylko do odczytu):
#   docker run --rm -v "$PWD:/src:ro" python:3.12-slim bash /src/docker/wheel/notebook-smoke.sh
# Bez argumentu wybiera jedyne kolo cp312-abi3 hosta z build/wheelhouse/.
# Lokalny build, interpreter zgodny z modulem, z numpy, torch, nbclient, ipykernel:
#   PYTHON=.venv-python/bin/python3 docker/wheel/notebook-smoke.sh --module-dir build/Debug-Python/python
# Odbior na prawdziwym Colabie pozostaje w #448; ten skrypt go nie zastepuje.
set -euo pipefail

repo_dir=$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)
mode=wheel
case $# in
  0)
    shopt -s nullglob
    wheels=("$repo_dir"/build/wheelhouse/retractordb-*-cp312-abi3-*_"$(uname -m)".whl)
    if [[ ${#wheels[@]} -ne 1 ]]; then
      echo "notebook-smoke: expected one cp312-abi3 $(uname -m) wheel in build/wheelhouse; pass a wheel or --module-dir DIR" >&2
      exit 2
    fi
    wheel=${wheels[0]}
    ;;
  1) wheel=$1 ;;
  2)
    if [[ $1 != --module-dir || ! -d $2 ]]; then
      echo "usage: $0 [wheel | --module-dir DIR]" >&2
      exit 2
    fi
    mode=module
    module_dir=$(cd "$2" && pwd)
    ;;
  *)
    echo "usage: $0 [wheel | --module-dir DIR]" >&2
    exit 2
    ;;
esac

work_dir=$(mktemp -d "${TMPDIR:-/tmp}/rdb-notebook-smoke.XXXXXX")
trap 'rm -rf -- "$work_dir"' EXIT

if [[ $mode == wheel ]]; then
  if [[ $(uname -s) != Linux ]]; then
    echo "notebook-smoke: Linux wheels require a Linux host or container" >&2
    exit 2
  fi
  [[ -f $wheel ]] || { echo "notebook-smoke: no such wheel: $wheel" >&2; exit 2; }
  wheel=$(cd "$(dirname "$wheel")" && pwd)/$(basename "$wheel")
  python=${PYTHON:-python3.12}
  "$python" -c 'import sys; sys.exit(sys.version_info[:2] != (3, 12))' || {
    echo "notebook-smoke: wheel profile requires Python 3.12; set PYTHON" >&2
    exit 2
  }
  "$python" -m venv "$work_dir/venv"
  python=$work_dir/venv/bin/python3
  # Staly profil lokalnej proby, bez deklarowania wersji aktualnego runtime Colab.
  "$python" -m pip install --quiet --disable-pip-version-check numpy==2.0.2 nbclient ipykernel
  torch_index=${RDB_TORCH_INDEX-https://download.pytorch.org/whl/cpu}
  torch_args=()
  if [[ -n $torch_index ]]; then torch_args=(--index-url "$torch_index"); fi
  "$python" -m pip install --quiet --disable-pip-version-check "${torch_args[@]}" torch==2.11.0
  export RDB_WHEEL=$wheel
  unset RDB_PYTHON_MODULE_DIR PYTHONPATH
else
  python=${PYTHON:-python3}
  export RDB_PYTHON_MODULE_DIR=$module_dir PYTHONPATH=$module_dir
  unset RDB_WHEEL
fi

export RDB_REPO_DIR=$repo_dir
# Wlasny katalog specyfikacji jadra: nie bierz kernelspec zostawionego przez hosta.
export JUPYTER_DATA_DIR=$work_dir/share/jupyter JUPYTER_PATH=$work_dir/share/jupyter
"$python" -m ipykernel install --prefix "$work_dir" --name rdb-smoke >/dev/null

"$python" - "$repo_dir" "$work_dir" "$mode" << 'PY'
import os
import re
import subprocess
import sys
from datetime import datetime
from pathlib import Path

import nbformat
from nbclient import NotebookClient

repo, work_dir, mode = Path(sys.argv[1]), Path(sys.argv[2]), sys.argv[3]
INSTALL_BUDGET_S = 20.0


def freeze():
    listing = subprocess.run(
        [sys.executable, '-m', 'pip', 'freeze', '--all', '--disable-pip-version-check'],
        check=True, capture_output=True, text=True,
    ).stdout
    return set(listing.splitlines())


def seconds(cell):
    timing = cell.metadata['execution']
    start, end = (datetime.fromisoformat(timing[k]) for k in ('iopub.execute_input', 'shell.execute_reply'))
    return (end - start).total_seconds()


before = freeze() if mode == 'wheel' else None
for name in ('colab_quickstart', 'j0_storage', 'j1_engine'):
    notebook = repo / 'api/python/notebooks' / (name + '.ipynb')
    nb = nbformat.read(notebook, as_version=4)
    nbformat.validate(nb)
    # Skladnia RQL w komorkach ma byc biezaca, takze gdy parser akceptuje stara.
    for cell in nb.cells:
        if cell.cell_type == 'code':
            assert not re.search(r'\bDECLARE\b[^\n]*\bFILE\s', cell.source), notebook
            assert not re.search(r'\.\s*(?:min|max|avg|sumc)\b', cell.source), notebook
    NotebookClient(nb, timeout=600, kernel_name='rdb-smoke', resources={'metadata': {'path': str(work_dir)}}).execute()
    for cell in nb.cells:
        if cell.cell_type != 'code':
            continue
        print(f'--- {name}/{cell.id}: {seconds(cell):.1f} s', flush=True)
        for output in cell.outputs:
            if output.output_type == 'stream':
                print(output.text, end='', flush=True)
    if name == 'colab_quickstart':
        if mode == 'wheel':
            after = freeze()
            added, removed = sorted(after - before), sorted(before - after)
            if removed or [re.split(r'[ =@]', line, maxsplit=1)[0].lower() for line in added] != ['retractordb']:
                sys.exit('notebook-smoke: pip changed more than adding retractordb:\n'
                         + ''.join(f'  - {line}\n' for line in removed)
                         + ''.join(f'  + {line}\n' for line in added))
            install = next(cell for cell in nb.cells if 'install' in cell.metadata.get('tags', []))
            if seconds(install) > INSTALL_BUDGET_S:
                sys.exit(f'notebook-smoke: install + import {seconds(install):.1f} s exceeds {INSTALL_BUDGET_S:g} s')
            print(f'notebook-smoke: install + import {seconds(install):.1f} s; pip added only retractordb', flush=True)
        # Import w procesie sterujacym dopiero po instalacji kola w notebooku.
        import retractordb._core as core
        expected = Path(core.__file__).resolve()
        print(f'notebook-smoke: selected _core={expected}', flush=True)
        # Pierwszy notebook tez musi wypisac sciezke wybranego rozszerzenia.
        assert any(str(expected) in output.get('text', '') for cell in nb.cells for output in cell.get('outputs', []))
        os.environ['RDB_EXPECTED_CORE'] = str(expected)
    print(f'notebook-smoke: {name} OK', flush=True)

print(f'notebook-smoke: OK - 3/3 notebooks ({mode})', flush=True)
PY
