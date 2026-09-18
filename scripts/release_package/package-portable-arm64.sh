#!/usr/bin/env bash
# Lokalna budowa AArch64 (portable .tar.gz) w Dockerze; wynik w build/ARM64-Packages.
exec "$(dirname "${BASH_SOURCE[0]}")/package-portable.sh" arm64 "$@"
