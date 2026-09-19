# shellcheck shell=bash

# PODLOGA wersji CMake, nie zamrozenie. Dryft narzedzia w gore jest normalny
# i ma przechodzic bez bledu i bez ostrzezenia - nowszy cmake spelnia ten prog.
# Chodzi wylacznie o to, zeby nie zejsc PONIZEJ wersji, dla ktorej zachowania
# polityk sa ustalone: zakres cmake_minimum_required(VERSION 3.20...4.4) w
# CMakeLists i dolna granica tool_requires w conanfile.py mowia to samo.
#
# Po co: dopoki conanfile mowil [>=3.25], `conan build` bral dowolna nowsza
# wersje, a `cmake` wolane wprost z PATH bylo tym z apt albo z venv - dwie rozne
# wersje generowaly to samo drzewo. 2026-08-18 wyszlo to jako ostrzezenie CMP0219
# (polityka od 4.4) widoczne wylacznie w CI i niereprodukowalne lokalnie (4.2.3).
# Gdy nowszy cmake zacznie ostrzegac o kolejnej polityce, podnosi sie gorna
# granice zakresu w CMakeLists - swiadomie, w commicie.
RDB_CMAKE_MIN_VERSION="4.4.2"

command_exists() {
    command -v "$1" >/dev/null 2>&1
}

# --- Platforma ----------------------------------------------------------
# Jedno miejsce odpowiadajace na pytanie "na czym to dziala". Pozostale moduly
# nie wolaja `uname` same z siebie - pytaja tutaj. Powod: macOS rozni sie od
# Linuksa w kilkunastu drobiazgach (rc powloki, menedzer pakietow, kompilator,
# licznik rdzeni, linker) i bez jednego punktu decyzyjnego rozeszloby sie to
# po wszystkich plikach.
rdb_os() {
    case "$(uname -s 2>/dev/null)" in
        Linux) echo "linux" ;;
        Darwin) echo "macos" ;;
        *) echo "unknown" ;;
    esac
}

# Sciezka do POKAZANIA w komunikacie: $HOME skracamy do '~'. Dzieki temu
# komunikat nazywa plik, ktorego naprawde uzyto, a na Linuksie brzmi dokladnie
# tak samo jak wtedy, gdy '~/.bashrc' bylo w nim wpisane na sztywno.
rdb_display_path() {
    if [ -n "${HOME:-}" ]; then
        case "$1" in
            "$HOME"/*)
                echo "~${1#$HOME}"
                return 0
                ;;
        esac
    fi
    echo "$1"
}

# Ktory plik rc powloki dopisujemy (PATH, aktywacja venv) - sam wybor, bez
# skutkow ubocznych. Linux: zawsze ~/.bashrc (bez zmian). macOS: logowanie
# idzie przez zsh, wiec ~/.zshrc; ~/.bash_profile tylko wtedy, gdy $SHELL
# jawnie wskazuje bash. Gdy $SHELL nic nie mowi, zostaje ~/.zshrc - to
# domyslna powloka macOS.
rdb_shell_rc_path() {
    if [ "$(rdb_os)" = "macos" ]; then
        case "${SHELL:-}" in
            */bash) echo "$HOME/.bash_profile" ;;
            *) echo "$HOME/.zshrc" ;;
        esac
    else
        echo "$HOME/.bashrc"
    fi
}

# To samo, ale plik po wyjsciu ma istniec - tak jak dotad tworzyl go
# ensure_single_bashrc_line dla ~/.bashrc.
rdb_shell_rc() {
    local rc
    rc=$(rdb_shell_rc_path)
    touch "$rc" 2>/dev/null || true
    echo "$rc"
}

# Instalacja pakietow systemowych. Linux: apt-get - dokladnie te same dwa
# wywolania, ktore staly wczesniej w install_missing_tools. macOS: brew i
# NIGDY przez sudo (Homebrew w /opt/homebrew jest per-uzytkownik i pod sudo
# odmawia pracy).
rdb_pkg_install() {
    [ $# -gt 0 ] || return 0
    if [ "$(rdb_os)" = "macos" ]; then
        if ! command_exists brew; then
            echo "Error: cannot auto-install packages without Homebrew. Missing packages: $*"
            echo "-- Install Homebrew from https://brew.sh and re-run."
            return 1
        fi
        brew install "$@"
    else
        if ! command_exists sudo || ! command_exists apt-get; then
            echo "Error: cannot auto-install apt packages without sudo and apt-get. Missing packages: $*"
            return 1
        fi
        # Acquire::Retries: lustra potrafia oddac 503 na POJEDYNCZYM pliku, a bez
        # ponowien wywraca to caly job. Na CI ARM (us-east-1.ec2.ports.ubuntu.com)
        # zdarza sie to okresowo na fonts-liberation, ciagnietym jako zaleznosc
        # graphviza z listy `toolchain_required`. apt ponawia samo POBRANIE pozycji,
        # wiec to jest ta warstwa, na ktorej awaria lustra ma byc obsluzona.
        sudo apt-get -o Acquire::Retries=5 update
        sudo apt-get -o Acquire::Retries=5 -y install "$@"
    fi
}

# Kompilator, ktorym sprawdzamy C++23 (check_cxx23 w toolmatrix.sh) i ktory
# nazywaja komunikaty bledow. Linux: zawsze `g++`. macOS: kompilatorem jest
# AppleClang, a `g++` to tylko jego shim - pytamy wiec o $CXX, domyslnie `c++`.
rdb_cxx23_probe_compiler() {
    if [ "$(rdb_os)" = "macos" ]; then
        echo "${CXX:-c++}"
    else
        echo "g++"
    fi
}

# Wersja kompilatora do raportu. AppleClang ODRZUCA -dumpfullversion (blad, nie
# pusty wynik - stad "missing/fail" w tabeli validate), wiec na macOS czytamy
# pierwsza linie `--version`.
rdb_cxx_version() {
    local cxx="$1"
    if [ "$(rdb_os)" = "macos" ]; then
        extract_first_version "$("$cxx" --version 2>/dev/null | head -n1)"
    else
        "$cxx" -dumpfullversion -dumpversion 2>/dev/null | head -n1
    fi
}

append_unique() {
    local value="$1"
    shift
    local existing
    for existing in "$@"; do
        if [ "$existing" = "$value" ]; then
            return 1
        fi
    done
    return 0
}

version_ge() {
    local have="$1"
    local want="$2"
    [ "$(printf '%s\n' "$want" "$have" | sort -V | head -n1)" = "$want" ]
}

# Dobiera bezpieczną liczbę równoległych zadań kompilacji na podstawie
# dostępnej pamięci RAM. Powód: pojedyncza jednostka kompilacji tego projektu
# (Boost + antlr4-runtime + GTest, C++23, -O3) potrafi zająć nawet ~800MB RSS
# mimo PCH -- zmierzone na wygenerowanych parserach ANTLR (RQLParser.cpp,
# DESCParser.cpp) oraz compiler.cpp/executorsm.cpp/expressionEvaluator.cpp.
# Domyślne '-j = liczba rdzeni' na maszynach z małą ilością RAM (np. RPi 400:
# 4 rdzenie / 3.7GB) doprowadziło w testach do peaku 3.1GB/3.7GB zużytej
# pamięci (poniżej 700MB wolnego) -- blisko swapowania/OOM. Formuła zakłada
# ~850MB na zadanie i zostawia ~500MB marginesu na system/bufory; na maszynach
# z dużą ilością RAM nie ogranicza niczego (wynik i tak ograniczony do liczby
# rdzeni). Nadpisanie ręczne: RDB_BUILD_JOBS=N w env, lub opcje
# 'lowmem'/'nolowmem' (patrz niżej, analogicznie do 'mold'/'nomold').
#
# TA SAMA REGUŁA ISTNIEJE DRUGI RAZ, NA SZTYWNO, W `.circleci/config.yml`.
# Kroki CI nie mogą wywołać funkcji z tego skryptu (każdy `run:` to osobna
# powłoka na czystym drzewie), a `$(nproc)` w kontenerze podaje rdzenie HOSTA,
# nie przydział kontenera -- 2026-08-19 job `research-gate` dostał 36 zadań przy
# 8 GiB executora `large` i został ubity przez OOM po 45 s, bez komunikatu.
# Joby `research-gate` i `build-release-ablation` mają więc wpisane `--parallel 4`,
# co odpowiada tej formule dla `resource_class: large` (4 vCPU / 8 GiB).
# Przy zmianie klasy executora albo przy zmianie założenia ~850MB/zadanie
# poprawić OBA miejsca -- tutaj i w `config.yml`.
compute_build_jobs() {
    if [ -n "${RDB_BUILD_JOBS:-}" ]; then
        echo "$RDB_BUILD_JOBS"
        return 0
    fi
    local nproc_count mem_kb mem_mb jobs_by_mem
    # Liczba rdzeni: `nproc` (Linux), `sysctl -n hw.ncpu` (macOS), a gdyby i
    # tego nie bylo - POSIX-owe getconf. Bez tego macOS schodzil cicho do
    # jednego zadania i kazdy build trwal wiecznosc.
    nproc_count=$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || getconf _NPROCESSORS_ONLN 2>/dev/null || echo 1)
    # RAM: /proc/meminfo (Linux, kB) albo `sysctl -n hw.memsize` (macOS, bajty).
    # Obie drogi normalizujemy do kB, zeby formula RAM-owa nizej pozostala ta
    # sama - to jest jedyne miejsce, ktore wie, skad wzielo sie `mem_kb`.
    mem_kb=$(awk '/MemTotal/{print $2}' /proc/meminfo 2>/dev/null || echo "")
    if [ -z "$mem_kb" ]; then
        mem_kb=$(( $(sysctl -n hw.memsize 2>/dev/null || echo 0) / 1024 ))
        if [ "$mem_kb" -le 0 ]; then
            mem_kb=""
        fi
    fi
    if [ -z "$mem_kb" ]; then
        echo "$nproc_count"
        return 0
    fi
    mem_mb=$((mem_kb / 1024))
    jobs_by_mem=$(( (mem_mb - 500) / 850 ))
    if [ "$jobs_by_mem" -lt 1 ]; then
        jobs_by_mem=1
    fi
    if [ "$jobs_by_mem" -lt "$nproc_count" ]; then
        echo "$jobs_by_mem"
    else
        echo "$nproc_count"
    fi
}

extract_first_version() {
    local text="$1"
    echo "$text" | grep -oE '[0-9]+([.][0-9]+)+' | head -n1
}

extract_major() {
    local ver="$1"
    echo "$ver" | cut -d. -f1
}

# Nazwa jest historyczna: funkcja od zawsze dostaje plik parametrem i nie wie
# nic o ~/.bashrc. Plik wskazuje rdb_shell_rc() - na macOS bedzie to ~/.zshrc
# albo ~/.bash_profile. Zmiana nazwy przeszlaby przez trzy moduly, wiec zostaje.
ensure_single_bashrc_line() {
    local file="$1"
    local desired_line="$2"
    local match_regex="$3"
    local tmp_file

    touch "$file"
    tmp_file=$(mktemp)

    awk -v desired_line="$desired_line" -v match_regex="$match_regex" '
        BEGIN {
            inserted = 0
        }
        {
            if ($0 == desired_line) {
                if (!inserted) {
                    print desired_line
                    inserted = 1
                }
                next
            }

            if ($0 ~ match_regex) {
                if (!inserted) {
                    print desired_line
                    inserted = 1
                }
                next
            }

            print
        }
        END {
            if (!inserted) {
                print desired_line
            }
        }
    ' "$file" > "$tmp_file"

    mv "$tmp_file" "$file"
}

run_common_option() {
    local opt="$1"
    case "$opt" in
        "mold")
            if [ "$(rdb_os)" = "macos" ]; then
                # mold nie linkuje Mach-O i nie bedzie. RDB_USE_MOLD zostaje
                # NIEUSTAWIONE, zeby CMake uzyl swojego domyslnego OFF spoza
                # Linuksa - wyeksportowanie ON tylko wywracaloby konfiguracje.
                echo "-- mold does not link Mach-O; ignoring 'mold' on macOS (RDB_USE_MOLD left unset, i.e. OFF)."
            else
                export RDB_USE_MOLD=ON
                echo "-- mold linker ENABLED for subsequent build options in this invocation."
            fi
            ;;
        "nomold")
            export RDB_USE_MOLD=OFF
            echo "-- mold linker DISABLED for subsequent build options in this invocation (falls back to default linker)."
            ;;
        "lowmem")
            export RDB_BUILD_JOBS=2
            echo "-- Manual override ENABLED: build parallelism fixed at 2 jobs, overriding the automatic RAM-aware default, for subsequent options in this invocation (e.g. RPi: buildrdb.sh lowmem release)."
            ;;
        "nolowmem")
            unset RDB_BUILD_JOBS
            echo "-- Manual override CLEARED: build parallelism reverts to the automatic RAM-aware default (always applied unless RDB_BUILD_JOBS is set) for subsequent options in this invocation."
            ;;
        "quit")
            echo "-- Current conan profile is:"
            cat ~/.conan2/profiles/default
            echo "-- Ok, quit - no action."
            ;;
    esac
}
