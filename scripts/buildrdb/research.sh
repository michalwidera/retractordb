# shellcheck shell=bash

probe_cmake_args=(
    -DRDB_OPT_DEDUP_SUBSTRATES=ON
    -DRDB_OPT_SHARE_EQUIVALENT_SELECTS=ON
    -DRDB_OPT_COMMUTATIVE_ADD=ON
    -DRDB_OPT_FACTOR_MATCHED_HASH_TIMEMOVES=ON
    -DRDB_OPT_SIMPLIFY_EXPRESSIONS=ON
    -DRDB_BENCH_PROBE=ON
)

toggle_on_off() {
    if [ "$1" = "ON" ]; then
        echo "OFF"
    else
        echo "ON"
    fi
}

choose_ablation_options() {
    local dedup="ON"
    local share="ON"
    local commutative="ON"
    local factor="ON"
    local probe="OFF"
    local simplify="ON"
    local choice

    while true; do
        echo ""
        echo "Release ablation configuration:"
        echo "  1) RDB_OPT_DEDUP_SUBSTRATES=$dedup"
        echo "  2) RDB_OPT_SHARE_EQUIVALENT_SELECTS=$share"
        echo "  3) RDB_OPT_COMMUTATIVE_ADD=$commutative"
        echo "  4) RDB_OPT_FACTOR_MATCHED_HASH_TIMEMOVES=$factor"
        echo "  5) RDB_BENCH_PROBE=$probe"
        echo "  6) RDB_OPT_SIMPLIFY_EXPRESSIONS=$simplify"
        echo "  b) Build selected configuration"
        echo "  q) Cancel"

        if ! read -r -p "-- Toggle option or build [1-6/b/q]: " choice; then
            choice="q"
        fi

        case "$choice" in
            1) dedup=$(toggle_on_off "$dedup") ;;
            2) share=$(toggle_on_off "$share") ;;
            3) commutative=$(toggle_on_off "$commutative") ;;
            4) factor=$(toggle_on_off "$factor") ;;
            5) probe=$(toggle_on_off "$probe") ;;
            6) simplify=$(toggle_on_off "$simplify") ;;
            b|B)
                if [ "$share" = "OFF" ] && [ "$commutative" = "ON" ]; then
                    echo "Error: RDB_OPT_COMMUTATIVE_ADD=ON requires RDB_OPT_SHARE_EQUIVALENT_SELECTS=ON."
                    echo "-- Toggle option 3 to OFF or option 2 back to ON."
                    continue
                fi

                ablation_cmake_args=(
                    "-DRDB_OPT_DEDUP_SUBSTRATES=$dedup"
                    "-DRDB_OPT_SHARE_EQUIVALENT_SELECTS=$share"
                    "-DRDB_OPT_COMMUTATIVE_ADD=$commutative"
                    "-DRDB_OPT_FACTOR_MATCHED_HASH_TIMEMOVES=$factor"
                    "-DRDB_OPT_SIMPLIFY_EXPRESSIONS=$simplify"
                    "-DRDB_BENCH_PROBE=$probe"
                )
                ablation_dedup="$dedup"
                ablation_share="$share"
                ablation_commutative="$commutative"
                ablation_factor="$factor"
                ablation_probe="$probe"
                ablation_simplify="$simplify"
                ablation_build_dir="$rdb_source_dir/build/Release-Ablation/dedup-${dedup}_share-${share}_comm-${commutative}_factor-${factor}_probe-${probe}_simplify-${simplify}"
                ablation_conan_dir="$rdb_source_dir/build/Conan-Release-Ablation/dedup-${dedup}_share-${share}_comm-${commutative}_factor-${factor}_probe-${probe}_simplify-${simplify}"
                return 0
                ;;
            q|Q)
                echo "-- Release ablation build cancelled."
                return 1
                ;;
            *) echo "Invalid choice: $choice" ;;
        esac
    done
}

run_research_option() {
    local opt="$1"
    case "$opt" in
        "release-ablation")
            if ! choose_ablation_options; then
                return 0
            fi

            echo "-- Selected ablation build directory: $ablation_build_dir"
            printf '%s\n' "${ablation_cmake_args[@]}"

            sed 's/Debug/Release/g' <~/.conan2/profiles/default >~/.conan2/profiles/temp && mv ~/.conan2/profiles/temp ~/.conan2/profiles/default
            conan source "$rdb_source_dir"
            conan install "$rdb_source_dir" -s build_type=Release --build missing -of "$ablation_conan_dir"

            cmake \
                -S "$rdb_source_dir" \
                -B "$ablation_build_dir" \
                -G Ninja \
                -DCMAKE_BUILD_TYPE=Release \
                -DCMAKE_TOOLCHAIN_FILE="$ablation_conan_dir/build/Release/generators/conan_toolchain.cmake" \
                "${ablation_cmake_args[@]}"

            build_jobs=$(compute_build_jobs)
            echo "-- Building ablation variant with -j$build_jobs"
            cmake --build "$ablation_build_dir" --parallel "$build_jobs"

            verify_optimizer_build_info \
                "$ablation_build_dir/src/retractor/xretractor" \
                "$ablation_dedup" \
                "$ablation_share" \
                "$ablation_commutative" \
                "$ablation_factor" \
                "$ablation_probe" \
                "$ablation_simplify"
            ;;
        "probe")
            # Budowa Release z WŁĄCZONĄ sondą pomiarową (benchmark E1/E3). Release, bo
            # sonda służy do pomiarów wydajności — mierzymy kod zoptymalizowany, nie Debug.
            # Osobny katalog nie pozwala tej binarce przeniknąć do build/Release.
            probe_build_dir="$rdb_source_dir/build/Release-Probe"
            probe_conan_dir="$rdb_source_dir/build/Conan-Release-Probe"
            sed 's/Debug/Release/g' <~/.conan2/profiles/default >~/.conan2/profiles/temp && mv ~/.conan2/profiles/temp ~/.conan2/profiles/default
            conan source "$rdb_source_dir"
            conan install "$rdb_source_dir" -s build_type=Release --build missing -of "$probe_conan_dir"
            cmake \
                -S "$rdb_source_dir" \
                -B "$probe_build_dir" \
                -G Ninja \
                -DCMAKE_BUILD_TYPE=Release \
                -DCMAKE_TOOLCHAIN_FILE="$probe_conan_dir/build/Release/generators/conan_toolchain.cmake" \
                "${probe_cmake_args[@]}"
            build_jobs=$(compute_build_jobs)
            echo "-- Building with -j$build_jobs (RAM-aware cap applied automatically by default; override with RDB_BUILD_JOBS=N, or force a fixed cap via 'lowmem')"
            cmake --build "$probe_build_dir" --parallel "$build_jobs"
            verify_optimizer_build_info \
                "$probe_build_dir/src/retractor/xretractor" \
                ON ON ON ON ON ON
            echo "-- [warning: probe benchmark build] sonda pomiarowa WŁĄCZONA w tej kompilacji (RDB_BENCH_PROBE=ON)."
            ;;
        "gate_requirements")
            # Zaleznosci bramki badawczej `ninja test_gate` (test/research_gate).
            #
            # Bramka ma trzy poziomy o roznych wymaganiach i ten punkt instaluje
            # to, czego brakuje najwyzszemu:
            #   1. H10 i podstawowe kontrole H9  -> python3 (zwykle juz jest);
            #   2. H9, 84 kompilacje             -> cztery profile ablacji;
            #   3. H9, oracle wartosci vs Flink  -> JDK 17 + Flink 2.3.0.
            gate_dir="$rdb_source_dir/test/research_gate"
            jdk_home="${JAVA_HOME_PINNED:-/usr/lib/jvm/java-17-openjdk-amd64}"
            flink_home="${FLINK_HOME:-$HOME/opt/flink-2.3.0}"
            flink_tgz="flink-2.3.0-bin-scala_2.12.tgz"
            flink_url="${FLINK_URL:-https://archive.apache.org/dist/flink/flink-2.3.0/$flink_tgz}"
            # Suma zweryfikowana na obu maszynach kampanii przed zapisaniem tutaj.
            flink_sha512="e5863767caeaa7c72e45fc62d45f7df9f435a1c83aed813ea550db39e9221194d148ea4a6c3bfb5604335974729c579d48c6c4c3eb43502e37310a0bf982462a"

            [ -d "$gate_dir" ] || { echo "Error: brak $gate_dir"; exit 1; }

            echo "-- 1/4 python3"
            command -v python3 >/dev/null || { echo "-- installing python3..."; sudo apt-get install -y python3; }
            echo "   OK: $(python3 --version)"

            # JDK 17 jest PRZYPIETY, nie 'jakas Java'. Domyslna java systemu bywa
            # nowsza i aparatura Flinka jej nie akceptuje.
            echo "-- 2/4 JDK 17 (przypiety: $jdk_home)"
            if [ ! -x "$jdk_home/bin/javac" ]; then
                echo "   brak, instaluje openjdk-17-jdk..."
                sudo apt-get install -y openjdk-17-jdk || { echo "Error: instalacja JDK 17 nie powiodla sie"; exit 1; }
            fi
            [ -x "$jdk_home/bin/javac" ] || { echo "Error: nadal brak $jdk_home/bin/javac"; exit 1; }
            echo "   OK: $("$jdk_home/bin/java" -version 2>&1 | head -1)"

            echo "-- 3/4 Flink 2.3.0 ($flink_home)"
            if [ ! -d "$flink_home/lib" ]; then
                mkdir -p "$(dirname "$flink_home")"
                tgz_path="$(dirname "$flink_home")/$flink_tgz"
                if [ ! -f "$tgz_path" ]; then
                    echo "   pobieram $flink_url (ok. 600 MB)..."
                    command -v curl >/dev/null || sudo apt-get install -y curl
                    curl -fL --retry 3 -o "$tgz_path" "$flink_url" \
                        || { echo "Error: pobranie Flinka nie powiodlo sie"; exit 1; }
                fi
                echo "   weryfikuje sume kontrolna..."
                actual_sha="$(sha512sum "$tgz_path" | cut -d' ' -f1)"
                if [ "$actual_sha" != "$flink_sha512" ]; then
                    echo "Error: suma SHA-512 archiwum Flinka sie nie zgadza."
                    echo "  oczekiwano: $flink_sha512"
                    echo "  otrzymano : $actual_sha"
                    echo "  Nie rozpakowuje. Usun $tgz_path i sprobuj ponownie."
                    exit 1
                fi
                echo "   suma zgodna, rozpakowuje..."
                tar xzf "$tgz_path" -C "$(dirname "$flink_home")" \
                    || { echo "Error: rozpakowanie nie powiodlo sie"; exit 1; }
            fi
            [ -f "$flink_home/lib/flink-dist-2.3.0.jar" ] \
                || { echo "Error: brak $flink_home/lib/flink-dist-2.3.0.jar"; exit 1; }
            echo "   OK: $flink_home"

            # Dowodem instalacji jest kompilacja klas bramki, nie obecnosc plikow.
            echo "-- 4/4 proba: kompilacja klas Flinka bramki H9"
            if JAVA_HOME_PINNED="$jdk_home" FLINK_HOME="$flink_home" \
               "$gate_dir/h9/flink/build_flink.sh" >/dev/null 2>&1; then
                echo "   OK: klasy skompilowane"
            else
                echo "Error: kompilacja klas Flinka nie powiodla sie; uruchom recznie:"
                echo "  JAVA_HOME_PINNED=$jdk_home FLINK_HOME=$flink_home \\"
                echo "    $gate_dir/h9/flink/build_flink.sh"
                exit 1
            fi

            echo ""
            echo "-- Gotowe. Bramka:"
            echo "     ninja test_gate                  (z build/Debug)"
            echo "     $gate_dir/run_gate.sh --only h10|h9 --count N --profiles DIR"
            echo ""
            # Binarka profilu nie niesie SHA zrodla, wiec swiezosc mierzymy czasem
            # modyfikacji. Profil starszy od src/ orzekalby o innej rewizji.
            profiles_state="ok"
            for p in DEFAULT NO_R2_CANON NO_R1_FACTOR NO_R1_NO_R2; do
                gate_bin="$rdb_source_dir/build/K26v3-$p/src/retractor/xretractor"
                if [ ! -x "$gate_bin" ]; then profiles_state="brak"; break; fi
                if [ -n "$(find "$rdb_source_dir/src" -type f -newer "$gate_bin" -print -quit 2>/dev/null)" ]; then
                    profiles_state="stare"; break
                fi
            done
            case "$profiles_state" in
                ok)
                    echo "-- Profile ablacji H9: obecne i nie starsze od zrodel."
                    echo "   Poziom 84/84: $gate_dir/run_gate.sh --only h9 --profiles $rdb_source_dir/build"
                    ;;
                stare)
                    echo "-- Uwaga: profile ablacji sa STARSZE od src/ — orzekalyby o innej"
                    echo "   rewizji niz badana. Bramka pominie poziom 84/84. Przebuduj:"
                    echo "     $gate_dir/h9/build_profiles.sh"
                    ;;
                *)
                    echo "-- Uwaga: brak kompletu czterech profili ablacji, wiec poziom H9"
                    echo "   '84/84 kompilacji' pozostanie POMINIETY. Zbuduj je przez:"
                    echo "     $gate_dir/h9/build_profiles.sh"
                    ;;
            esac
            ;;
    esac
}
