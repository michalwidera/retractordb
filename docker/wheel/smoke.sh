#!/usr/bin/env bash
# Proba dymna srodowiska budowy kol (docker/wheel/Dockerfile). Buduje _core z
# biezacego drzewa w obrazie i sprawdza naraz trzy rzeczy, od ktorych zalezy J4:
#   1. cache Conana w obrazie jest kompletny wzgledem conanfile.py (--build never),
#   2. modul spelnia polityke manylinux obrazu (audit-so.py, czyli auditwheel),
#   3. testy Pythona przechodza na glibc 2.28, a nie tylko na macOS.
#
# Uruchamiac WEWNATRZ obrazu, z korzeniem repo zamontowanym tylko do odczytu
# (--platform i tag obrazu zgodne z architektura, patrz naglowek Dockerfile):
#
#   docker run --rm --platform linux/amd64 -v "$PWD:/src:ro" \
#     buildenv-retractordb-wheel:manylinux_2_28_x86_64 bash /src/docker/wheel/smoke.sh
#
# Drzewo jest kopiowane do kontenera tak jak w scripts/test-ci.sh (git ls-files
# -co --exclude-standard), wiec nic nie trafia do katalogu na hoscie, a build/
# i .venv-python z macOS nie mieszaja sie z buildem linuksowym.
set -euo pipefail

readonly src_dir=/src
# Colab: Python 3.12 (runtime 2026.07). Te same pakiety co scripts/python-venv.sh
# poza narzedziami do formatowania.
readonly python=/opt/python/cp312-cp312/bin/python3
readonly venv_dir=/tmp/rdb-smoke-venv

[[ -e "$src_dir/.git" ]] || {
  echo "smoke: $src_dir is not the repository root - mount it with -v \"\$PWD:/src:ro\"" >&2
  exit 2
}

work_dir=$(mktemp -d /tmp/rdb-wheel-smoke.XXXXXX)
# safe.directory: montowanie ma wlasciciela z hosta, a kontener dziala jako root.
# GIT_OPTIONAL_LOCKS=0: repo jest tylko do odczytu, git nie ma czego blokowac.
GIT_OPTIONAL_LOCKS=0 git -c safe.directory="$src_dir" -C "$src_dir" ls-files -co --exclude-standard -z \
  | tar -C "$src_dir" --null --files-from - --ignore-failed-read --create --file - \
  | tar -C "$work_dir" --extract --file -
cd "$work_dir"

"$python" -m venv "$venv_dir"
"$venv_dir/bin/pip" install --quiet --disable-pip-version-check "nanobind>=2.0" "numpy>=1.23" "pytest>=7.0"

# --build never: brak pakietu w cache obrazu ma byc bledem, a nie cicha budowa
# w kontenerze - to jest wlasnie pytanie, czy obraz nadaza za conanfile.py.
conan install . -s build_type=Release --build never
cmake -S . -B build/Release -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_TOOLCHAIN_FILE=build/Release/generators/conan_toolchain.cmake \
  -DRDB_PYTHON=ON \
  -DPython_EXECUTABLE="$venv_dir/bin/python3"

# Rownoleglosc wedlug pamieci, nie rdzeni: ninja bierze domyslnie nproc+2
# zadania, a pod Docker Desktop na Apple silicon (kilkanascie rdzeni, kilka GiB
# pamieci maszyny wirtualnej) kompilacja compiler.cpp z -O3 ginela od OOM
# killera. 2 GiB na zadanie to proporcja executora CI (4 vCPU / 8 GiB,
# scripts/test-ci.sh). Limit cgroup (docker run --memory; v2 albo v1) wygrywa z
# MemTotal, bo /proc/meminfo pokazuje pamiec calej maszyny wirtualnej.
mem_kib=$(awk '/^MemTotal:/ {print $2}' /proc/meminfo)
cgroup_max=$(cat /sys/fs/cgroup/memory.max 2> /dev/null ||
  cat /sys/fs/cgroup/memory/memory.limit_in_bytes 2> /dev/null || echo max)
if [[ $cgroup_max =~ ^[0-9]+$ ]] && ((cgroup_max / 1024 < mem_kib)); then
  mem_kib=$((cgroup_max / 1024))
fi
jobs=$((mem_kib / (2 * 1024 * 1024)))
jobs=$((jobs < 1 ? 1 : jobs))
jobs=$((jobs > $(nproc) ? $(nproc) : jobs))
echo "smoke: building _core with $jobs jobs ($((mem_kib / 1024 / 1024)) GiB, $(nproc) CPUs)"
cmake --build build/Release --target _core --parallel "$jobs"

/opt/rdb-tools/venv/bin/python3 /opt/rdb-tools/audit-so.py build/Release/python/retractordb/_core*.so
# conftest.py sam znajduje build/Release/python.
"$venv_dir/bin/python3" -m pytest -q api/python/tests
echo "smoke: OK ($AUDITWHEEL_PLAT, $("$python" --version))"
