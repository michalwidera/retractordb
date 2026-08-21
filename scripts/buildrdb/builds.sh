# shellcheck shell=bash

production_cmake_args=(
    -DRDB_OPT_DEDUP_SUBSTRATES=ON
    -DRDB_OPT_SHARE_EQUIVALENT_SELECTS=ON
    -DRDB_OPT_COMMUTATIVE_ADD=ON
    -DRDB_OPT_FACTOR_MATCHED_HASH_TIMEMOVES=ON
    -DRDB_OPT_SIMPLIFY_EXPRESSIONS=ON
    -DRDB_BENCH_PROBE=OFF
)

require_pristine_source_tree() {
    local source_status

    if ! git -C "$rdb_source_dir" rev-parse --is-inside-work-tree >/dev/null 2>&1; then
        echo "Error: production release requires a Git working tree."
        return 1
    fi

    source_status=$(git -C "$rdb_source_dir" status --porcelain=v1 --untracked-files=all)
    if [ -n "$source_status" ]; then
        echo "Error: production release rejected because the source tree is not pristine:"
        printf '%s\n' "$source_status"
        echo "-- Commit, remove or stash every tracked, staged and untracked change before building release."
        echo "-- To build a NON-production Release from the tree as it is, use 'release-dirty'."
        return 1
    fi
}

# Wariant kontroli dla 'release-dirty'. Drzewo wolno mieć zmiany, ale sam build
# nadal nie może go ruszać — dlatego zamiast pustego statusu porównujemy stan
# z migawką wykonaną przed pierwszym krokiem.
release_dirty_baseline=""

require_unmodified_source_tree() {
    local source_status

    if ! git -C "$rdb_source_dir" rev-parse --is-inside-work-tree >/dev/null 2>&1; then
        echo "Error: release build requires a Git working tree."
        return 1
    fi

    source_status=$(git -C "$rdb_source_dir" status --porcelain=v1 --untracked-files=all)
    if [ "$source_status" != "$release_dirty_baseline" ]; then
        echo "Error: the build modified the source tree:"
        diff <(printf '%s\n' "$release_dirty_baseline") <(printf '%s\n' "$source_status") || true
        return 1
    fi
}

# Bramka wybierana przez tryb: 'release' wymaga czystego drzewa,
# 'release-dirty' wyłącznie niezmienionego.
release_source_tree_guard() {
    if [ "$release_allow_dirty" = "ON" ]; then
        require_unmodified_source_tree
    else
        require_pristine_source_tree
    fi
}

run_with_sanitized_build_environment() {
    env \
        -u CFLAGS \
        -u CPPFLAGS \
        -u CXXFLAGS \
        -u LDFLAGS \
        -u CMAKE_ARGS \
        -u CMAKE_GENERATOR \
        -u CMAKE_TOOLCHAIN_FILE \
        -u RDB_BENCH_CSV \
        -u RDB_BENCH_PLAN \
        "$@"
}

verify_optimizer_build_info() {
    local binary="$1"
    local dedup="$2"
    local share="$3"
    local commutative="$4"
    local factor="$5"
    local probe="$6"
    local simplify="$7"
    local actual
    local expected

    if [ ! -x "$binary" ]; then
        echo "Error: expected xretractor binary not found: $binary"
        return 1
    fi

    actual=$("$binary" --build-info)
    expected=$(printf '%s\n' \
        "RDB_OPT_DEDUP_SUBSTRATES=$dedup" \
        "RDB_OPT_SHARE_EQUIVALENT_SELECTS=$share" \
        "RDB_OPT_COMMUTATIVE_ADD=$commutative" \
        "RDB_OPT_FACTOR_MATCHED_HASH_TIMEMOVES=$factor" \
        "RDB_BENCH_PROBE=$probe" \
        "RDB_OPT_SIMPLIFY_EXPRESSIONS=$simplify")

    if [ "$actual" != "$expected" ]; then
        echo "Error: built xretractor configuration does not match the selected build mode."
        echo "-- Expected:"
        printf '%s\n' "$expected"
        echo "-- Actual:"
        printf '%s\n' "$actual"
        return 1
    fi

    # Konfiguracja domyślna (wszystkie optymalizacje ON, sonda OFF) jest cicha —
    # raportujemy tylko odchylenia: wyłączoną optymalizację albo włączoną sondę.
    if [ "$dedup" != "ON" ] || [ "$share" != "ON" ] || [ "$commutative" != "ON" ] || [ "$factor" != "ON" ] ||
        [ "$simplify" != "ON" ] || [ "$probe" != "OFF" ]; then
        echo "-- Verified xretractor build configuration (non-default):"
        printf '%s\n' "$actual"
    fi
}

run_build_option() {
    local opt="$1"
    case "$opt" in
        "release"|"release-dirty")
            # Produkcyjny Release powstaje wyłącznie z czystego drzewa źródeł,
            # świeżego katalogu i jawnej konfiguracji. Typowe zmienne wstrzykujące
            # flagi są usuwane z procesu, a gotowa binarka jest kontrolowana poniżej.
            # Wariant 'release-dirty' buduje z drzewa takiego, jakie jest — służy do
            # weryfikacji zmian przed commitem i NIE jest wydaniem produkcyjnym.
            if [ "$opt" = "release-dirty" ]; then
                release_allow_dirty=ON
                release_dirty_baseline=$(git -C "$rdb_source_dir" status --porcelain=v1 --untracked-files=all)
                echo "-- release-dirty: building from a MODIFIED source tree; the result is NOT a production release."
                if [ -n "$release_dirty_baseline" ]; then
                    printf '%s\n' "$release_dirty_baseline"
                fi
            else
                release_allow_dirty=OFF
            fi
            release_source_tree_guard
            cmake -E remove_directory "$rdb_source_dir/build/Release"
            sed 's/Debug/Release/g' <~/.conan2/profiles/default >~/.conan2/profiles/temp && mv ~/.conan2/profiles/temp ~/.conan2/profiles/default
            run_with_sanitized_build_environment conan source "$rdb_source_dir"
            release_source_tree_guard
            run_with_sanitized_build_environment conan install "$rdb_source_dir" -s build_type=Release --build missing
            run_with_sanitized_build_environment cmake \
                -S "$rdb_source_dir" \
                -B "$rdb_source_dir/build/Release" \
                -G Ninja \
                -DCMAKE_BUILD_TYPE=Release \
                -DCMAKE_TOOLCHAIN_FILE="$rdb_source_dir/build/Release/generators/conan_toolchain.cmake" \
                "${production_cmake_args[@]}"
            build_jobs=$(compute_build_jobs)
            echo "-- Building with -j$build_jobs (RAM-aware cap applied automatically by default; override with RDB_BUILD_JOBS=N, or force a fixed cap via 'lowmem')"
            run_with_sanitized_build_environment cmake --build "$rdb_source_dir/build/Release" --parallel "$build_jobs"
            verify_optimizer_build_info \
                "$rdb_source_dir/build/Release/src/retractor/xretractor" \
                ON ON ON ON OFF ON
            release_source_tree_guard
            ;;
        "debug")
            sed 's/Release/Debug/g' <~/.conan2/profiles/default >~/.conan2/profiles/temp && mv ~/.conan2/profiles/temp ~/.conan2/profiles/default
            conan source "$rdb_source_dir"
            conan install "$rdb_source_dir" -s build_type=Debug --build missing
            build_jobs=$(compute_build_jobs)
            echo "-- Building with -j$build_jobs (RAM-aware cap applied automatically by default; override with RDB_BUILD_JOBS=N, or force a fixed cap via 'lowmem')"
            export CMAKE_BUILD_PARALLEL_LEVEL="$build_jobs"
            conan build "$rdb_source_dir" -s build_type=Debug --build missing
            ;;
        "package")
            # Pakowanie binarne (DEB;TGZ wg CPACK_GENERATOR) z auto-sprzątaniem
            # śmieci stagingu. Wybiera istniejący katalog buildu (Release > Debug),
            # jawnie przywraca konfigurację produkcyjną i przebudowuje binaria, aby
            # lepki cache po badaniu ablacyjnym nie trafił do pakietu.
            cd "$rdb_source_dir" || exit 1
            if   [ -d build/Release ]; then pkg_dir="build/Release"
            elif [ -d build/Debug ];   then pkg_dir="build/Debug"
            else echo "Error: no build dir found. Run 'debug' or 'release' first."; exit 1
            fi
            cmake -S "$rdb_source_dir" -B "$pkg_dir" "${production_cmake_args[@]}"
            build_jobs=$(compute_build_jobs)
            cmake --build "$pkg_dir" --parallel "$build_jobs"
            cd "$pkg_dir" || exit 1
            cpack || echo "-- cpack zgłosił błędy (np. brak dpkg-deb dla DEB) — sprawdzam wynik."
            # Śmieci po packagingu: katalog stagingu i manifest instalacji. Finalne
            # archiwa (.deb/.tar.gz) zostają.
            rm -rf _CPack_Packages install_manifest.txt
            echo "-- Packaging artifacts in $pkg_dir:"
            ls -1 *.deb *.tar.gz 2>/dev/null || echo "   (none produced)"
            ;;
        "coverage")
            gcc_ver=$(gcc -dumpversion | cut -d. -f1)
            gcov_exec="gcov-${gcc_ver}"
            echo "-- GCC $gcc_ver detected, checking coverage tools..."

            # gcov — optional install path: install only when missing.
            if ! command -v "$gcov_exec" &>/dev/null; then
                echo "-- $gcov_exec not found, installing $gcov_exec..."
                sudo apt-get install -y gcc-${gcc_ver} || { echo "Error: Failed to install $gcov_exec"; exit 1; }

                # Verify only when we had to install.
                if ! command -v "$gcov_exec" &>/dev/null; then
                    echo "Error: $gcov_exec still not available after install attempt"; exit 1
                fi
            fi

            gcov_ver=$("$gcov_exec" --version | head -1 | grep -oP '\d+' | head -1)
            echo "-- OK: $gcov_exec version $gcov_ver matches GCC $gcc_ver"

            # gcovr — narzędzie raportujące
            if ! command -v gcovr &>/dev/null; then
                echo "-- gcovr not found, installing..."
                pip3 install gcovr || { echo "Error: Failed to install gcovr"; exit 1; }
            fi

            cd "$rdb_source_dir"
            conan install "$rdb_source_dir" -s build_type=Debug --build missing
            cmake \
                -S "$rdb_source_dir" \
                -B "$rdb_source_dir/build/Debug" \
                -G Ninja \
                -DCMAKE_BUILD_TYPE=Debug \
                -DCMAKE_TOOLCHAIN_FILE="$rdb_source_dir/build/Debug/generators/conan_toolchain.cmake" \
                -DENABLE_COVERAGE=ON
            cd build/Debug
            find . -name '*.gcda' -delete -o -name '*.gcno' -delete
            build_jobs=$(compute_build_jobs)
            echo "-- Building with -j$build_jobs (RAM-aware cap applied automatically by default; override with RDB_BUILD_JOBS=N, or force a fixed cap via 'lowmem')"
            ninja clean && ninja -j "$build_jobs"
            export PATH="$(pwd)/src/retractor:$(pwd)/src/rdb:$(pwd)/src/qry:$PATH"
            ctest || true
            cd ../..
            rm -f *.gcov
            mkdir -p coverage
            gcovr --root . --filter 'src/' --gcov-executable "$gcov_exec" --gcov-ignore-errors all --exclude '.*\.antlr.*' build/Debug --html-details coverage/coverage.html --xml coverage/coverage.xml --print-summary
            ;;
    esac
}
