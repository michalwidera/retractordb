# shellcheck shell=bash

# Modul sprawdzac przez `shellcheck -x scripts/buildrdb.sh` (analiza calego
# programu). Osobno zglasza SC2154/SC2034 na zmiennych dzielonych przez
# zasieg dynamiczny -- to nie jest defekt, tylko brak kontekstu.
#
# Czego wymaga ktora opcja i czy narzedzie jest juz obecne. Modul jest sama
# wiedza: nic nie instaluje i nic nie wypisuje na wyjscie uzytkownika.

# Compile and run a small C++23 probe.
# Tests std::ranges::fold_left, the uz size_t literal and std::println (<print>)
# — all C++23 and used in the codebase.
# Requires GCC 14+ (libstdc++ 13 has no <print> at all).  Returns 0 on success.
check_cxx23() {
    local tmpdir rc
    tmpdir=$(mktemp -d)
    cat > "$tmpdir/cxx23check.cpp" << 'EOF'
#include <algorithm>
#include <cstddef>
#include <print>
#include <vector>
int main() {
  std::vector<int> v{1, 2, 3};
  std::size_t s = std::ranges::fold_left(v, 0uz,
      [](std::size_t a, int x){ return a + std::size_t(x); });
  std::println("{}", s);
  return s == 6 ? 0 : 1;
}
EOF
    g++ -std=c++23 -o "$tmpdir/cxx23check" "$tmpdir/cxx23check.cpp" 2>/dev/null \
        && "$tmpdir/cxx23check" >/dev/null 2>&1
    rc=$?
    rm -rf "$tmpdir"
    return $rc
}

find_python3() {
    local ver py
    for ver in 3.13 3.12 3.11 3.10 3.9 3.8; do
        py="python${ver}"
        if command_exists "$py"; then
            echo "$py"
            return 0
        fi
    done
    if command_exists python3; then
        echo "python3"
        return 0
    fi
    return 1
}

tool_installed() {
    local tool="$1"
    case "$tool" in
        cmake-pinned)
            # Sama obecnosc `cmake` nie wystarcza — liczy sie, czy w PATH stoi
            # wersja nie starsza niz podloga. Nowsza spelnia warunek i NIE jest
            # cofana do podlogi: dryft w gore jest oczekiwany.
            local ver
            command_exists cmake || return 1
            ver=$(extract_first_version "$(cmake --version 2>/dev/null | head -n1)")
            [ -n "$ver" ] && version_ge "$ver" "$RDB_CMAKE_MIN_VERSION"
            ;;
        graphviz)
            command_exists dot
            ;;
        clang-tidy)
            command_exists clang-tidy || compgen -G '/usr/bin/clang-tidy-*' >/dev/null
            ;;
        python3-venv)
            local py
            py=$(find_python3 2>/dev/null) || return 1
            "$py" -m venv --help >/dev/null 2>&1 && "$py" -c 'import ensurepip' >/dev/null 2>&1
            ;;
        build-essential)
            dpkg -s build-essential >/dev/null 2>&1
            ;;
        batcat)
            command_exists batcat || command_exists bat
            ;;
        conan)
            command_exists conan || [ -x "$HOME/.venv/bin/conan" ]
            ;;
        *)
            command_exists "$tool"
            ;;
    esac
}

cmd_to_apt_package() {
    local cmd="$1"
    case "$cmd" in
        gcc) echo "gcc" ;;
        g++) echo "g++" ;;
        cmake) echo "cmake" ;;
        ninja) echo "ninja-build" ;;
        make) echo "make" ;;
        git) echo "git" ;;
        gdb) echo "gdb" ;;
        python3) echo "python3" ;;
        python3-venv) echo "" ;;
        pip3) echo "python3-pip" ;;
        build-essential) echo "build-essential" ;;
        valgrind) echo "valgrind" ;;
        cppcheck) echo "cppcheck" ;;
        mold) echo "mold" ;;
        ccache) echo "ccache" ;;
        graphviz) echo "graphviz" ;;
        feh) echo "feh" ;;
        tmux) echo "tmux" ;;
        gnuplot) echo "gnuplot" ;;
        clang-format) echo "clang-format" ;;
        clang-tidy) echo "clang-tidy" ;;
        shellcheck) echo "shellcheck" ;;
        rg) echo "ripgrep" ;;
        hexdump) echo "bsdextrautils" ;;
        apt-get) echo "apt" ;;
        sudo) echo "sudo" ;;
        batcat) echo "bat" ;;
        conan|gcovr|cmake-format|cmake-pinned) echo "" ;;
        *) echo "$cmd" ;;
    esac
}

# Tabela wymagan: dla opcji `opt` ustawia `tool_specs` (i `validate_only` dla
# opcji `validate`) w zasiegu wywolujacego -- patrz kontrakt zmiennych w
# ensure_tools_for_option.
tool_specs_for_option() {
    local opt="$1"

    case "$opt" in
        "release"|"release-dirty"|"release-ablation"|"debug"|"probe")
            tool_specs=(
                "gcc:required" "g++:required" "cmake:required"
                "ninja:required" "conan:required" "make:required"
            )
            ;;
        "conan")
            tool_specs=("gcc:required" "g++:required" "conan:required")
            ;;
        "ninja")
            tool_specs=("conan:required")
            ;;
        "coverage")
            tool_specs=(
                "gcc:required" "g++:required" "cmake:required" "ninja:required"
                "conan:required" "pip3:required" "gcovr:required" "valgrind:required"
            )
            ;;
        "toolchain")
            tool_specs=(
                "sudo:required" "apt-get:required"
                "git:required" "gcc:required" "g++:required"
                "cmake:required" "cmake-pinned:required" "make:required" "ninja:required"
                "python3:required" "python3-venv:required" "pip3:required" "conan:required"
                "valgrind:required" "hexdump:required"
                "graphviz:recommended"
                "cppcheck:recommended" "mold:recommended" "ccache:recommended" "rg:recommended"
                "cmake-format:optional" "clang-format:optional" "clang-tidy:optional" "shellcheck:optional" "gdb:optional" "tmux:optional" "feh:optional" "gnuplot:optional"
            )
            ;;
        "toolchain_required")
            tool_specs=(
                "sudo:required" "apt-get:required"
                "gcc:required" "g++:required" "cmake:required" "cmake-pinned:required"
                "make:required" "ninja:required" "build-essential:required"
                "python3:required" "python3-venv:required" "pip3:required"
                "mold:required" "valgrind:required"
                "hexdump:required" "conan:required"
                # graphviz OPCJONALNY na tej liscie (minimalny toolchain CI), a nie
                # wymagany: jedynym jego konsumentem jest render SVG w tescie
                # it_issue31_doc, ktory bez `dot` sam sie nie rejestruje. Na obrazie
                # projektu graphviz jest, wiec x86 nie traci nic. Joby uzywajace tej
                # listy (ARM, czysta Ubuntu) przestaja przez to ciagnac zaleznosci
                # graphviza z apt — a to wlasnie na nich lustro ports.ubuntu.com
                # oddawalo okresowo 503 na fonts-liberation.
                #
                # POZIOM TRZYMAC SPOJNIE Z LISTA `validate`: ona ma wlasny zestaw
                # i sprawdza go NIEZALEZNIE od tego, co ktorykolwiek `toolchain`
                # zainstalowal. 2026-08-19 obnizenie poziomu tylko tutaj wywrocilo
                # `manual-arm` na kroku "Validate installed toolchain": pakiet nie
                # byl juz instalowany, a validate nadal go wymagal.
                "graphviz:optional"
                "ccache:optional" "clang-format:optional"
            )
            ;;
        "toolchain_all")
            tool_specs=(
                "sudo:required" "apt-get:required"
                "git:required" "gcc:required" "g++:required"
                "cmake:required" "cmake-pinned:required" "make:required" "ninja:required"
                "python3:required" "python3-venv:required" "pip3:required" "conan:required"
                "valgrind:required" "hexdump:required" "graphviz:required"
                "cppcheck:required" "gdb:required" "mold:required" "ccache:required" "cmake-format:required" "clang-format:required" "clang-tidy:required" "shellcheck:required" "rg:required"
                "tmux:required" "feh:required" "gnuplot:required"
            )
            ;;
        "validate")
            tool_specs=(
                "git:required" "gcc:required" "g++:required"
                "cmake:required" "make:required" "conan:required" "ninja:required"
                "python3:required" "pip3:required" "valgrind:required"
                "hexdump:required"
                "graphviz:recommended"
                "cppcheck:recommended" "mold:recommended" "ccache:recommended" "rg:recommended"
                "cmake-format:optional" "clang-format:optional" "clang-tidy:optional" "shellcheck:optional" "gdb:optional" "tmux:optional" "feh:optional" "gnuplot:optional"
            )
            validate_only=1
            ;;
        "batsyntax")
            tool_specs=("batcat:required")
            ;;
        "attach_knowledge")
            tool_specs=("git:required")
            ;;
        "bashrc"|"vimsyntax"|"quit"|"help"|"--help"|"-h")
            tool_specs=()
            ;;
        *)
            tool_specs=()
            ;;
    esac
}
