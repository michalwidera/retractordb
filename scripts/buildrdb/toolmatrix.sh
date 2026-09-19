# shellcheck shell=bash

# Modul sprawdzac przez `shellcheck -x scripts/buildrdb.sh` (analiza calego
# programu). Osobno zglasza SC2154/SC2034 na zmiennych dzielonych przez
# zasieg dynamiczny -- to nie jest defekt, tylko brak kontekstu.
#
# Czego wymaga ktora opcja i czy narzedzie jest juz obecne. Modul jest sama
# wiedza: nic nie instaluje i nic nie wypisuje na wyjscie uzytkownika.

# Compile and run a small C++23 probe.
# Tests std::ranges::fold_left, the uz size_t literal and std::println (<print>)
# - all C++23 and used in the codebase.
# The compiler is whatever rdb_cxx23_probe_compiler names: g++ on Linux, where
# this still means GCC 14+ (libstdc++ 13 has no <print> at all), and ${CXX:-c++}
# on macOS, where the compiler is AppleClang and `g++` is only a shim for it -
# there is no GCC version floor to speak of there.  Returns 0 on success.
check_cxx23() {
    local tmpdir rc cxx
    cxx=$(rdb_cxx23_probe_compiler)
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
    "$cxx" -std=c++23 -o "$tmpdir/cxx23check" "$tmpdir/cxx23check.cpp" 2>/dev/null \
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
            # Sama obecnosc `cmake` nie wystarcza - liczy sie, czy w PATH stoi
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

# Odpowiednik tabeli apt dla Homebrew. Pusta nazwa znaczy to samo co wyzej:
# narzedzie ma wlasny instalator (pip/venv) albo jest czescia systemu.
# gcc/g++ na macOS to shimy AppleClanga z Xcode CLT - nie ma pakietu brew,
# ktory mialoby sens tu podstawic (`brew install gcc` dalby PRAWDZIWE GCC, a
# projekt buduje sie tu AppleClangiem), wiec obsluguje je osobno
# install_missing_special_tool.
cmd_to_brew_package() {
    local cmd="$1"
    case "$cmd" in
        gcc) echo "" ;;
        g++) echo "" ;;
        cmake) echo "cmake" ;;
        ninja) echo "ninja" ;;
        make) echo "make" ;;
        git) echo "git" ;;
        gdb) echo "gdb" ;;
        python3) echo "python" ;;
        python3-venv) echo "" ;;
        pip3) echo "python" ;;
        cppcheck) echo "cppcheck" ;;
        ccache) echo "ccache" ;;
        graphviz) echo "graphviz" ;;
        dot) echo "graphviz" ;;
        tmux) echo "tmux" ;;
        gnuplot) echo "gnuplot" ;;
        clang-format) echo "clang-format" ;;
        clang-tidy) echo "llvm" ;;
        shellcheck) echo "shellcheck" ;;
        rg) echo "ripgrep" ;;
        bat|batcat) echo "bat" ;;
        # hexdump jest czescia bazowego macOS - nie ma czego instalowac.
        hexdump) echo "" ;;
        conan|gcovr|cmake-format|cmake-pinned) echo "" ;;
        *) echo "$cmd" ;;
    esac
}

# Jedno wejscie dla obu tabel: kto pyta o pakiet, dostaje nazwe wlasciwa dla
# tej platformy. Na Linuksie to dokladnie cmd_to_apt_package, jak dotad.
cmd_to_platform_package() {
    if [ "$(rdb_os)" = "macos" ]; then
        cmd_to_brew_package "$1"
    else
        cmd_to_apt_package "$1"
    fi
}

# Narzedzia, ktorych na tej platformie NIE MA I NIE BEDZIE. To nie jest "brak"
# - to "nie dotyczy": mold nie linkuje Mach-O, valgrind nie wspiera arm64 macOS,
# build-essential/apt-get/sudo/setcap to swiat dpkg, a feh to X11. Traktowanie
# ich jako brakow oznaczaloby, ze `validate` na macOS zawsze konczy sie bledem,
# a `toolchain*` w kolko probuje instalowac nieistniejace pakiety.
rdb_tool_not_applicable() {
    local cmd="$1"
    [ "$(rdb_os)" = "macos" ] || return 1
    case "$cmd" in
        mold|valgrind|build-essential|sudo|apt-get|setcap|feh) return 0 ;;
        *) return 1 ;;
    esac
}

# Demotuje wpisy niedotyczace platformy do poziomu `n/a`, ktory
# ensure_tools_for_option raportuje i pomija. Robimy to JEDNYM przebiegiem na
# koncu tabeli zamiast rozgalezniac kazda liste z osobna - inaczej `mold` i
# `valgrind` (wpisane jako `required` w toolchain_required, toolchain_all,
# coverage i validate) wywracalyby macOS w czterech miejscach naraz.
rdb_demote_not_applicable_specs() {
    local spec cmd
    local -a filtered
    [ "$(rdb_os)" = "macos" ] || return 0
    filtered=()
    for spec in "${tool_specs[@]}"; do
        cmd=${spec%%:*}
        if rdb_tool_not_applicable "$cmd"; then
            filtered+=("$cmd:n/a")
        else
            filtered+=("$spec")
        fi
    done
    tool_specs=("${filtered[@]}")
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
                # graphviza z apt - a to wlasnie na nich lustro ports.ubuntu.com
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

    rdb_demote_not_applicable_specs
}
