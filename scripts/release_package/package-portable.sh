#!/usr/bin/env bash
# Budowa przenosnej paczki (portable .tar.gz) w Dockerze dla wskazanej architektury;
# dla x86_64 dodatkowo .deb. Wywolywany przez package-portable-arm64.sh i
# package-portable-x86.sh. --fresh czysci katalog roboczy kontenera (cache Conana
# zostaje), aby pliki usuniete z repozytorium nie przetrwaly w buildzie - tak
# buduje release.sh.
set -euo pipefail

usage() { echo "Usage: $0 arm64|x86_64 [--fresh]" >&2; exit 2; }

case "${1:-}" in
  arm64) platform=linux/arm64 machine=aarch64 elf_machine=183 dir_prefix=ARM64 with_deb=0 ;;
  x86_64) platform=linux/amd64 machine=x86_64 elf_machine=62 dir_prefix=X86 with_deb=1 ;;
  *) usage ;;
esac
arch=$1
fresh=0
case "${2:-}" in
  "") ;;
  --fresh) fresh=1 ;;
  *) usage ;;
esac

repo_dir=$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)
out_dir="$repo_dir/build/$dir_prefix-Packages"
cache_dir="$repo_dir/build/$dir_prefix-Conan-Cache"
work_cache_dir="$repo_dir/build/$dir_prefix-Work"
image="retractordb-portable-$arch:local"

command -v docker >/dev/null || { echo "Docker is required for this local $machine build." >&2; exit 1; }
if ((fresh)); then
  rm -rf -- "$work_cache_dir"
fi
mkdir -p "$out_dir" "$cache_dir" "$work_cache_dir"
work_location_file="$work_cache_dir/.container-work-path"
if [[ ! -f $work_location_file ]]; then
  printf '%s\n' "/tmp/rdb-$arch-work" > "$work_location_file"
fi
container_work_dir=$(cat "$work_location_file")
[[ $container_work_dir =~ ^/tmp/rdb-${arch}[-.][A-Za-z0-9]+$ ]] || {
  echo "Invalid $machine work directory marker." >&2
  exit 1
}
stage_dir=$(mktemp -d "$out_dir/.stage.XXXXXX")
trap 'rm -rf -- "$stage_dir"' EXIT

docker build --platform "$platform" -t "$image" \
  -f "$repo_dir/packaging/portable/Dockerfile" "$repo_dir"

docker run --rm --platform "$platform" \
  --user "$(id -u):$(id -g)" \
  -e HOME=/conan-cache \
  -e RDB_USE_MOLD=OFF \
  -e RDB_USE_CCACHE=OFF \
  -e RDB_WORK_DIR="$container_work_dir" \
  -e RDB_MACHINE="$machine" \
  -e RDB_ELF_MACHINE="$elf_machine" \
  -e RDB_WITH_DEB="$with_deb" \
  -v "$repo_dir:/src:ro" \
  -v "$cache_dir:/conan-cache" \
  -v "$work_cache_dir:$container_work_dir" \
  -v "$stage_dir:/out" \
  "$image" bash -euo pipefail -c '
    test "$(uname -m)" = "$RDB_MACHINE"
    mkdir -p "$HOME"
    work_dir="$RDB_WORK_DIR"
    mkdir -p "$work_dir"
    tar -C /src --exclude=./build --exclude=./test/research_gate/h10/work \
      -cf - . | tar -C "$work_dir" -xf -
    cd "$work_dir"
    conan profile detect -f
    sed -i "s/compiler.cppstd=gnu17/compiler.cppstd=gnu23/" "$HOME/.conan2/profiles/default"
    cmake_version=$(cmake --version | head -n1 | awk "{print \$3}")
    printf "\n[platform_tool_requires]\ncmake/%s\n" "$cmake_version" >> "$HOME/.conan2/profiles/default"
    conan install . -s build_type=Release --build missing
    cmake -S . -B build/Release -G Ninja \
      -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_TOOLCHAIN_FILE=build/Release/generators/conan_toolchain.cmake
    cmake --build build/Release --target package-portable --parallel 4
    # Nazwa z VERSION, nie wzorzec: katalog roboczy moze trzymac paczki starszych wersji.
    version=$(cat VERSION)
    archive="build/Release/retractordb-$version-linux-$RDB_MACHINE-portable.tar.gz"
    test -f "$archive"
    python3 - "$archive" "$RDB_ELF_MACHINE" <<"PY"
import struct, sys, tarfile
with tarfile.open(sys.argv[1], "r:gz") as package:
    for name in ("bin/xretractor", "bin/xqry", "bin/xtrdb"):
        with package.extractfile(name) as source:
            header = source.read(20)
        if header[:4] != b"\x7fELF" or struct.unpack_from("<H", header, 18)[0] != int(sys.argv[2]):
            sys.exit(f"{name} is not {sys.argv[2]} ELF machine")
PY
    package_dir=$(mktemp -d /tmp/rdb-portable-package.XXXXXX)
    tar -xzf "$archive" -C "$package_dir"
    # Wydanie ma produkcyjny zestaw przelacznikow, jak buildrdb.sh release.
    build_info=$("$package_dir/bin/xretractor" --build-info)
    expected=$(printf "%s\n" \
      RDB_OPT_DEDUP_SUBSTRATES=ON \
      RDB_OPT_SHARE_EQUIVALENT_SELECTS=ON \
      RDB_OPT_COMMUTATIVE_ADD=ON \
      RDB_OPT_FACTOR_MATCHED_HASH_TIMEMOVES=ON \
      RDB_BENCH_PROBE=OFF \
      RDB_OPT_SIMPLIFY_EXPRESSIONS=ON)
    if [ "$build_info" != "$expected" ]; then
      printf "Unexpected xretractor --build-info:\n%s\n" "$build_info" >&2
      exit 1
    fi
    "$package_dir/bin/xqry" -h >/dev/null
    "$package_dir/bin/xtrdb" -h >/dev/null
    cp "$archive" /out/
    if [ "$RDB_WITH_DEB" = 1 ]; then
      # cpack wprost, nie cel "package": ten przebudowuje cale all razem z testami.
      (cd build/Release && cpack -G DEB)
      deb="build/Release/retractordb-$version-Linux.deb"
      test -f "$deb"
      test "$(dpkg-deb -f "$deb" Architecture)" = "$(dpkg --print-architecture)"
      cp "$deb" /out/
    fi
  '

packages=()
while IFS= read -r -d '' package; do
  packages+=("$package")
done < <(find "$stage_dir" -maxdepth 1 -type f \( -name '*.tar.gz' -o -name '*.deb' \) -print0)
((${#packages[@]} == 1 + with_deb)) || { echo "$machine packages were not produced." >&2; exit 1; }
for package in "${packages[@]}"; do
  mv -f -- "$package" "$out_dir/"
  sha256sum "$out_dir/$(basename "$package")"
done
