# shellcheck shell=bash

# Modul sprawdzac przez `shellcheck -x scripts/buildrdb.sh` (analiza calego
# programu). Osobno zglasza SC2154/SC2034 na zmiennych dzielonych przez
# zasieg dynamiczny -- to nie jest defekt, tylko brak kontekstu.
#
# Raport opcji `validate`: podsumowanie brakow i tabela zgodnosci wersji.
# Nic nie instaluje. Czyta `missing_*`, `compat_failures` i `validation_failed`
# z zasiegu wywolujacego; zwraca 1, gdy brakuje narzedzia wymaganego.
report_validation_status() {
    printf -- "-----------------+--------------+-----------\n"
    echo "-- Summary: missing required=${#missing_required[@]}, recommended=${#missing_recommended[@]}, optional=${#missing_optional[@]}"
    if [ ${#missing_required[@]} -eq 0 ]; then
        echo "-- Environment validation passed for required build tools."
    else
        echo "-- Environment validation failed for required tools: ${missing_required[*]}"
        validation_failed=1
    fi
    if [ ${#missing_recommended[@]} -gt 0 ]; then
        echo "-- Missing recommended tools: ${missing_recommended[*]}"
    fi
    if [ ${#missing_optional[@]} -gt 0 ]; then
        echo "-- Missing optional tools: ${missing_optional[*]}"
    fi

    echo "-- Compatibility checks:"
    printf "%-28s | %-14s | %-8s | %-18s\n" "check" "installed" "status" "requirement"
    printf -- "-----------------------------+----------------+----------+-------------------\n"

    gcc_ver=""
    if command_exists gcc; then
        gcc_ver=$(gcc -dumpfullversion -dumpversion 2>/dev/null | head -n1)
        if [ -n "$gcc_ver" ] && version_ge "$gcc_ver" "14"; then
            compat_status="ok"
        else
            compat_status="fail"
            compat_failures=$((compat_failures + 1))
        fi
        printf "%-28s | %-14s | %-8s | %-18s\n" "gcc version" "${gcc_ver:-unknown}" "$compat_status" ">= 14"
    else
        compat_status="fail"
        compat_failures=$((compat_failures + 1))
        printf "%-28s | %-14s | %-8s | %-18s\n" "gcc version" "missing" "$compat_status" ">= 14"
    fi

    cmake_ver=""
    if command_exists cmake; then
        cmake_ver=$(extract_first_version "$(cmake --version 2>/dev/null | head -n1)")
        if [ -n "$cmake_ver" ] && version_ge "$cmake_ver" "3.20"; then
            compat_status="ok"
        else
            compat_status="fail"
            compat_failures=$((compat_failures + 1))
        fi
        printf "%-28s | %-14s | %-8s | %-18s\n" "cmake version" "${cmake_ver:-unknown}" "$compat_status" ">= 3.20"
    else
        compat_status="fail"
        compat_failures=$((compat_failures + 1))
        printf "%-28s | %-14s | %-8s | %-18s\n" "cmake version" "missing" "$compat_status" ">= 3.20"
    fi

    # Osobny wiersz, bo to inne pytanie niz "czy w ogole da sie zbudowac".
    # Tutaj chodzi o to, czy `cmake` z PATH nie jest STARSZY niz wersja, dla
    # ktorej zachowania polityk sa ustalone. Nowszy jest w porzadku — dryft
    # narzedzia w gore jest oczekiwany i nie jest ani bledem, ani ostrzezeniem.
    # Starszy jest tylko ostrzezeniem: build i tak przejdzie, bo conan wciaga
    # wlasnego cmake, ale drzewo generuja wtedy dwie rozne wersje — a wlasnie
    # tak powstalo ostrzezenie CMP0219 widoczne wylacznie w CI (2026-08-18).
    if [ -n "$cmake_ver" ] && version_ge "$cmake_ver" "$RDB_CMAKE_MIN_VERSION"; then
        compat_status="ok"
    else
        compat_status="warn"
    fi
    printf "%-28s | %-14s | %-8s | %-18s\n" "cmake policy floor" "${cmake_ver:-unknown}" "$compat_status" ">= $RDB_CMAKE_MIN_VERSION"

    conan_ver=""
    conan_bin=$(command -v conan 2>/dev/null || echo "$HOME/.venv/bin/conan")
    if [ -x "$conan_bin" ]; then
        conan_ver=$(extract_first_version "$("$conan_bin" --version 2>/dev/null)")
        if [ -n "$conan_ver" ] && version_ge "$conan_ver" "2.0"; then
            compat_status="ok"
        else
            compat_status="fail"
            compat_failures=$((compat_failures + 1))
        fi
        printf "%-28s | %-14s | %-8s | %-18s\n" "conan version" "${conan_ver:-unknown}" "$compat_status" ">= 2.0"
    else
        compat_status="fail"
        compat_failures=$((compat_failures + 1))
        printf "%-28s | %-14s | %-8s | %-18s\n" "conan version" "missing" "$compat_status" ">= 2.0"
    fi

    if command_exists g++ && check_cxx23; then
        compat_status="ok"
    else
        compat_status="fail"
        compat_failures=$((compat_failures + 1))
    fi
    gpp_ver="unknown"
    if command_exists g++; then
        gpp_ver=$(g++ -dumpfullversion -dumpversion 2>/dev/null | head -n1)
    fi
    printf "%-28s | %-14s | %-8s | %-18s\n" "g++ c++23 probe" "${gpp_ver:-unknown}" "$compat_status" "must pass"

    if [ -n "$gcc_ver" ]; then
        gcc_major=$(extract_major "$gcc_ver")
        gcov_exec="gcov-${gcc_major}"
        gcov_ver=""
        if command_exists "$gcov_exec"; then
            gcov_ver=$(extract_first_version "$("$gcov_exec" --version 2>/dev/null | head -n1)")
            gcov_major=$(extract_major "$gcov_ver")
            if [ -n "$gcov_major" ] && [ "$gcov_major" = "$gcc_major" ]; then
                compat_status="ok"
            else
                compat_status="fail"
                compat_failures=$((compat_failures + 1))
            fi
            printf "%-28s | %-14s | %-8s | %-18s\n" "gcov/gcc major match" "${gcov_ver:-unknown}" "$compat_status" "gcov-${gcc_major}"
        else
            compat_status="warn"
            printf "%-28s | %-14s | %-8s | %-18s\n" "gcov/gcc major match" "missing" "$compat_status" "gcov-${gcc_major}"
        fi
    else
        compat_status="warn"
        printf "%-28s | %-14s | %-8s | %-18s\n" "gcov/gcc major match" "skipped" "$compat_status" "need gcc"
    fi

    printf -- "-----------------------------+----------------+----------+-------------------\n"
    if [ "$compat_failures" -eq 0 ]; then
        echo "-- Compatibility checks passed."
    else
        echo "-- Compatibility check warnings: $compat_failures"
    fi

    echo "-- Run '$0 toolchain' to auto-install required tools and choose recommended/optional installs."
    if [ "$validation_failed" -eq 1 ]; then
        return 1
    fi
    return 0
}
