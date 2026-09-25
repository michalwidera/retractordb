#!/usr/bin/env bash
# Kola Linux (J4) z biezacego drzewa: cibuildwheel w obrazie z docker/wheel/Dockerfile,
# konfiguracja w pyproject.toml (tool.cibuildwheel).
#
# Uzycie, z dowolnego katalogu (na hoscie potrzebny Docker i cibuildwheel):
#   docker/wheel/build-wheels.sh aarch64 | x86_64 | all
# Wynik: wheelhouse/ w korzeniu repo. cibuildwheel instaluje sie np. przez
# `pipx install cibuildwheel`; inna komende podaje sie w zmiennej CIBUILDWHEEL,
# np. CIBUILDWHEEL="uvx cibuildwheel".
#
# Dlaczego przez kopie: cibuildwheel kopiuje do kontenera CALY katalog, z ktorego
# startuje - razem z build/ (setki MB) i z _core zbudowanym na macOS, ktory
# conftest.py znalazlby w build/*/python i probowal zaladowac na Linuksie. Kopia
# plikow sledzonych i nowych nieignorowanych (jak w scripts/test-ci.sh) ma
# kilkanascie MB i odpowiada checkoutowi w CI.
#
# Obrazy: pyproject.toml wskazuje micwide/buildenv-retractordb-wheel:<tag>.
# Przed wypchnieciem na Docker Hub wystarczy lokalny `docker tag` (naglowek
# docker/wheel/Dockerfile) - obraz obecny lokalnie nie jest pobierany.
#
# Pisane pod bash 3.2 (domyslny na macOS) i pod tar z macOS (bsdtar).
set -euo pipefail

case "${1:-}" in
  aarch64 | x86_64) archs=$1 ;;
  all) archs=x86_64,aarch64 ;;
  *)
    echo "usage: $0 aarch64|x86_64|all" >&2
    exit 2
    ;;
esac

repo_dir=$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)
read -r -a cibuildwheel <<< "${CIBUILDWHEEL:-cibuildwheel}"
command -v "${cibuildwheel[0]}" > /dev/null || {
  echo "error: '${cibuildwheel[0]}' not found - install cibuildwheel (pipx install cibuildwheel) or set CIBUILDWHEEL" >&2
  exit 1
}

work_dir=$(mktemp -d "${TMPDIR:-/tmp}/rdb-wheels.XXXXXX")
trap 'rm -rf -- "$work_dir"' EXIT

# `git ls-files -c` wypisuje takze pliki skasowane w drzewie roboczym. test-ci.sh
# pomija je przez --ignore-failed-read GNU tara; bsdtar tej opcji nie ma, wiec
# odsiewamy je przed tarem.
(
  cd "$repo_dir"
  git ls-files -co --exclude-standard -z |
    while IFS= read -r -d '' path; do
      if [ -e "$path" ]; then printf '%s\0' "$path"; fi
    done |
    tar --null -T - -cf -
) | tar -C "$work_dir" -xf -

mkdir -p "$repo_dir/wheelhouse"
echo "build-wheels: $archs from $(git -C "$repo_dir" rev-parse --short HEAD) plus uncommitted changes -> $repo_dir/wheelhouse"
cd "$work_dir"
"${cibuildwheel[@]}" --platform linux --archs "$archs" --output-dir "$repo_dir/wheelhouse"
