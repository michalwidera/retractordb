#!/usr/bin/env bash
# Lokalna budowa x86_64 (portable .tar.gz i .deb) w Dockerze; wynik w build/X86-Packages.
exec "$(dirname "${BASH_SOURCE[0]}")/package-portable.sh" x86_64 "$@"
