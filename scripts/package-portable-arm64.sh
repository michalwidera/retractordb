#!/usr/bin/env bash
set -euo pipefail

repo_dir=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
out_dir="$repo_dir/build/ARM64-Packages"
cache_dir="$repo_dir/build/ARM64-Conan-Cache"
work_cache_dir="$repo_dir/build/ARM64-Work"
image='retractordb-portable-arm64:local'

command -v docker >/dev/null || { echo 'Docker is required for this local ARM64 build.' >&2; exit 1; }
mkdir -p "$out_dir" "$cache_dir" "$work_cache_dir"
work_location_file="$work_cache_dir/.container-work-path"
if [[ ! -f $work_location_file ]]; then
  printf '%s\n' '/tmp/rdb-arm64-work' > "$work_location_file"
fi
container_work_dir=$(cat "$work_location_file")
[[ $container_work_dir =~ ^/tmp/rdb-arm64[-.][A-Za-z0-9]+$ ]] || {
  echo 'Invalid ARM64 work directory marker.' >&2
  exit 1
}
stage_dir=$(mktemp -d "$out_dir/.stage.XXXXXX")
trap 'rm -rf -- "$stage_dir"' EXIT

docker build --platform linux/arm64 -t "$image" \
  -f "$repo_dir/packaging/portable-arm64/Dockerfile" "$repo_dir"

docker run --rm --platform linux/arm64 \
  --user "$(id -u):$(id -g)" \
  -e HOME=/conan-cache \
  -e RDB_USE_MOLD=OFF \
  -e RDB_USE_CCACHE=OFF \
  -e RDB_ARM_WORK="$container_work_dir" \
  -v "$repo_dir:/src:ro" \
  -v "$cache_dir:/conan-cache" \
  -v "$work_cache_dir:$container_work_dir" \
  -v "$stage_dir:/out" \
  "$image" bash -euo pipefail -c '
    test "$(uname -m)" = aarch64
    mkdir -p "$HOME"
    work_dir="$RDB_ARM_WORK"
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
    archive=$(find build/Release -maxdepth 1 -name "retractordb-*-linux-aarch64-portable.tar.gz" -print -quit)
    test -n "$archive"
    python3 - "$archive" <<"PY"
import struct, sys, tarfile
with tarfile.open(sys.argv[1], "r:gz") as package:
    for name in ("bin/xretractor", "bin/xqry", "bin/xtrdb"):
        with package.extractfile(name) as source:
            header = source.read(20)
        if header[:4] != b"\x7fELF" or struct.unpack_from("<H", header, 18)[0] != 183:
            sys.exit(f"{name} is not AArch64 ELF")
PY
    package_dir=$(mktemp -d /tmp/rdb-arm64-package.XXXXXX)
    tar -xzf "$archive" -C "$package_dir"
    "$package_dir/bin/xretractor" --build-info >/dev/null
    "$package_dir/bin/xqry" -h >/dev/null
    "$package_dir/bin/xtrdb" -h >/dev/null
    cp "$archive" /out/
  '

package=$(find "$stage_dir" -maxdepth 1 -name 'retractordb-*-linux-aarch64-portable.tar.gz' -print -quit)
[[ -n $package ]] || { echo 'ARM64 package was not produced.' >&2; exit 1; }
mv -f -- "$package" "$out_dir/"
sha256sum "$out_dir/$(basename "$package")"
