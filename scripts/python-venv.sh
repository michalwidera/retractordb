#!/bin/bash
# Srodowisko wirtualne Pythona dla modulu osadzonego (RDB_PYTHON=ON).
#
# Po co osobny skrypt: Python z Homebrew odmawia instalacji do srodowiska
# systemowego (PEP 668, "externally-managed-environment"), a --break-system-packages
# psuje instalacje Homebrew. Tworzymy wiec venv w drzewie i podajemy CMake
# sciezke do JEGO interpretera - nanobind instaluje sie do site-packages
# konkretnego interpretera, wiec wybor interpretera jest wyborem nanobinda.
#
# Skrypt jest idempotentny: przy istniejacym venv tylko doinstalowuje pakiety.
#
# Uzycie:
#   scripts/python-venv.sh                    # utworz/odswiez .venv-python
#   scripts/python-venv.sh --recreate         # skasuj i zbuduj od zera
#   scripts/python-venv.sh --python python3.12
#   scripts/python-venv.sh --venv /sciezka/do/venv
#   scripts/python-venv.sh --print-python     # wypisz sama sciezke interpretera
#
# Dalej: docs/build-options.md

set -o errexit
set -o nounset
set -o pipefail

script_dir=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
source_dir=$(cd "$script_dir/.." && pwd)

# Prog 3.10 jest wspolny dla nas, dla nanobinda od 3.0 i dla requires-python
# w api/python/pyproject.toml. Nie rozjezdzac go w jednym miejscu.
readonly python_minimum="3.10"
# nanobind, numpy i pytest sluza modulowi osadzonemu; clang-format i cmakelang obsluguja
# `ninja cformat`, ktory src/CMakeLists.txt kaze instalowac przez pipa - a tej drogi
# PEP 668 juz nie przepuszcza. Jeden venv zalatwia oba przypadki.
#
# clang-format PRZYPIETY do wersji z CLAUDE.md. Nie jest to ostroznosc na wyrost:
# kolejne wydania formatuja inaczej, wiec niepinowana wersja znaczy, ze `ninja
# cformat` u dwoch osob daje dwa rozne drzewa i roznica ta wchodzi do commitow.
# numpy: jedyna zaleznosc uruchomieniowa modulu osadzonego (Engine.to_numpy/window,
# docs/jupyter-integration.md sekcja 4); torch CELOWO nie - retractordb.torch importuje
# go dopiero na zyczenie, a test, ktory go potrzebuje, pomija sie bez niego.
readonly package_list=(
  "nanobind>=2.0"
  "numpy>=1.23"
  "pytest>=7.0"
  "clang-format==21.1.7"
  "cmakelang"
)

venv_dir="${RDB_PYTHON_VENV:-$source_dir/.venv-python}"
python_requested=""
recreate=0
print_only=0

while [ $# -gt 0 ]; do
  case "$1" in
    --recreate) recreate=1 ;;
    --print-python) print_only=1 ;;
    --python)
      # $# -ge 2, nie `shift ||`: shift ostatniego argumentu KONCZY SIE SUKCESEM,
      # wiec sprawdzenie po nim nie wychwytuje brakujacej wartosci, a `set -o
      # nounset` zabija skrypt komunikatem o numerze linii zamiast o opcji.
      [ $# -ge 2 ] || { echo "error: --python needs an interpreter" >&2; exit 2; }
      python_requested="$2"
      shift
      ;;
    --venv)
      [ $# -ge 2 ] || { echo "error: --venv needs a directory" >&2; exit 2; }
      venv_dir="$2"
      shift
      ;;
    -h | --help)
      sed -n '2,19p' "${BASH_SOURCE[0]}" | sed 's/^# \{0,1\}//'
      exit 0
      ;;
    *)
      echo "error: unknown option '$1' (try --help)" >&2
      exit 2
      ;;
  esac
  shift
done

venv_python="$venv_dir/bin/python3"

say() { [ "$print_only" -eq 1 ] || printf '%s\n' "$*"; }

# Czy dany interpreter spelnia prog wersji. Porownanie robi sam Python, a nie
# sort -V na napisach: "3.9" kontra "3.10" wychodzi tam odwrotnie.
version_ok() {
  "$1" - "$python_minimum" << 'PY' > /dev/null 2>&1
import sys
floor = tuple(int(part) for part in sys.argv[1].split("."))
sys.exit(0 if sys.version_info[:len(floor)] >= floor else 1)
PY
}

pick_interpreter() {
  if [ -n "$python_requested" ]; then
    command -v "$python_requested" > /dev/null 2>&1 || {
      echo "error: interpreter not found: $python_requested" >&2
      exit 1
    }
    version_ok "$python_requested" || {
      echo "error: $python_requested is older than Python $python_minimum" >&2
      exit 1
    }
    command -v "$python_requested"
    return
  fi

  # Od najnowszego: nowszy CPython buduje szybciej i daje nanobindowi stabilne
  # ABI, a przy braku wskazania i tak trzeba wybrac jeden w sposob powtarzalny.
  local candidate
  for candidate in python3.14 python3.13 python3.12 python3.11 python3.10 python3; do
    if command -v "$candidate" > /dev/null 2>&1 && version_ok "$candidate"; then
      command -v "$candidate"
      return
    fi
  done

  echo "error: no Python $python_minimum or newer found on PATH" >&2
  echo "hint:  brew install python@3.12   (macOS)" >&2
  exit 1
}

if [ "$recreate" -eq 1 ] && [ -d "$venv_dir" ]; then
  say "Removing $venv_dir"
  rm -rf "$venv_dir"
fi

if [ ! -x "$venv_python" ]; then
  base_python=$(pick_interpreter)
  say "Creating venv with $base_python ($("$base_python" --version 2>&1))"
  "$base_python" -m venv "$venv_dir"
else
  say "Reusing venv at $venv_dir ($("$venv_python" --version 2>&1))"
fi

# --disable-pip-version-check: wewnatrz venv ostrzezenie o wersji pip jest
# szumem, a skrypt ma wypisac na koncu polecenia, nie dopisek instalatora.
say "Installing: ${package_list[*]}"
if [ "$print_only" -eq 1 ]; then
  "$venv_python" -m pip install --quiet --upgrade --disable-pip-version-check pip > /dev/null
  "$venv_python" -m pip install --quiet --upgrade --disable-pip-version-check "${package_list[@]}" > /dev/null
else
  "$venv_python" -m pip install --upgrade --disable-pip-version-check pip
  "$venv_python" -m pip install --upgrade --disable-pip-version-check "${package_list[@]}"
fi

# Dokladnie ta proba, ktora wykonuje src/python/CMakeLists.txt. Sprawdzamy ja
# tutaj, zeby blad konfiguracji nie wyszedl dopiero w cmake.
if ! nanobind_cmake_dir=$("$venv_python" -m nanobind --cmake_dir 2>/dev/null); then
  echo "error: nanobind installed but '-m nanobind --cmake_dir' failed" >&2
  exit 1
fi

if [ "$print_only" -eq 1 ]; then
  printf '%s\n' "$venv_python"
  exit 0
fi

cat << EOF

Ready.
  interpreter    $venv_python
  nanobind cmake $nanobind_cmake_dir
EOF

# Konkretny katalog builda, a nie zastepnik w nawiasach ostrych. `cmake .` w
# korzeniu repozytorium znaczy konfiguracje W ZRODLACH, czego to drzewo nie
# obsluguje - katalog builda tworzy conan wraz z plikiem toolchain. Wypisujemy
# wiec polecenie dla kazdego istniejacego builda, zeby nie bylo co interpretowac.
found_build=0
for cache in "$source_dir"/build/*/CMakeCache.txt; do
  [ -f "$cache" ] || continue
  build_dir=$(dirname "$cache")
  found_build=1
  cat << EOF

Configure that build directory (re-runs cmake in place, keeps the conan toolchain):

  cd "$build_dir"
  cmake -DRDB_PYTHON=ON -DPython_EXECUTABLE="$venv_python" .
  ninja
EOF
done

if [ "$found_build" -eq 0 ]; then
  cat << EOF

No configured build directory found under build/. Create one first:

  scripts/buildrdb.sh conan ninja debug

then re-run this script for the exact configure command.
EOF
fi

cat << EOF

Run the tests from the repository root, with this interpreter:

  "$venv_python" -m pytest api/python/tests

Formatting: \`ninja cformat\` calls bare clang-format and cmake-format, so give it
this environment's bin directory on PATH:

  PATH="$venv_dir/bin:\$PATH" ninja cformat

Passing -DPython_EXECUTABLE is what makes this work without activating the venv;
CMake caches it, so later builds in that directory need nothing further.
EOF
