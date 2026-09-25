#!/usr/bin/env bash
# Liczba zadan kompilacji dla budowy w kontenerze z docker/wheel/Dockerfile:
# wedlug pamieci, nie rdzeni. Wypisuje sama liczbe, zeby dalo sie jej uzyc w $(...).
#
# ninja bierze domyslnie nproc+2 zadania, a pod Docker Desktop na Apple silicon
# (kilkanascie rdzeni, kilka GiB pamieci maszyny wirtualnej) kompilacja
# compiler.cpp z -O3 ginela od OOM killera. 2 GiB na zadanie to proporcja
# executora CI (4 vCPU / 8 GiB, scripts/test-ci.sh). Limit cgroup (docker run
# --memory; v2 albo v1) wygrywa z MemTotal, bo /proc/meminfo pokazuje pamiec
# calej maszyny wirtualnej.
#
# Uzywaja: smoke.sh oraz pyproject.toml (tool.cibuildwheel.linux, zmienna
# CMAKE_BUILD_PARALLEL_LEVEL).
set -euo pipefail

mem_kib=$(awk '/^MemTotal:/ {print $2}' /proc/meminfo)
cgroup_max=$(cat /sys/fs/cgroup/memory.max 2> /dev/null ||
  cat /sys/fs/cgroup/memory/memory.limit_in_bytes 2> /dev/null || echo max)
if [[ $cgroup_max =~ ^[0-9]+$ ]] && ((cgroup_max / 1024 < mem_kib)); then
  mem_kib=$((cgroup_max / 1024))
fi
jobs=$((mem_kib / (2 * 1024 * 1024)))
jobs=$((jobs < 1 ? 1 : jobs))
jobs=$((jobs > $(nproc) ? $(nproc) : jobs))
echo "$jobs"
