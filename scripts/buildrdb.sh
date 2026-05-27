#!/bin/bash

# https://zfredenburg.medium.com/force-a-bash-script-to-exit-on-error-ec50b374c98d
set -o errexit

foldername=${PWD##*/}          # to assign to a variable
foldername=${foldername:-/}    # to correct for the case where PWD=/

echo "-- Last two lines of ~/.bashrc are:"
tail -n 2 ~/.bashrc

case "$foldername" in
  "retractordb") build_folder=".";;
  "scripts")     build_folder="..";;
  "build")       build_folder="..";;
  "Release")     build_folder="../..";;
  "Debug")       build_folder="../..";;
  *)             echo "Unknown current build folder << $foldername >>"
                 exit
                 ;;
esac

echo "-- Note: Current folder is [ $foldername ] and will start build in [ $build_folder ]"

# Compile and run a small C++23 probe.
# Tests std::ranges::fold_left and the uz size_t literal — both C++23 and used in the codebase.
# Requires GCC 13+.  Returns 0 on success.
check_cxx23() {
    local tmpdir rc
    tmpdir=$(mktemp -d)
    cat > "$tmpdir/cxx23check.cpp" << 'EOF'
#include <algorithm>
#include <cstddef>
#include <vector>
int main() {
  std::vector<int> v{1, 2, 3};
  std::size_t s = std::ranges::fold_left(v, 0uz,
      [](std::size_t a, int x){ return a + std::size_t(x); });
  return s == 6 ? 0 : 1;
}
EOF
    g++ -std=c++23 -o "$tmpdir/cxx23check" "$tmpdir/cxx23check.cpp" 2>/dev/null \
        && "$tmpdir/cxx23check"
    rc=$?
    rm -rf "$tmpdir"
    return $rc
}

command_exists() {
    command -v "$1" >/dev/null 2>&1
}

tool_installed() {
    local tool="$1"
    case "$tool" in
        graphviz)
            command_exists dot
            ;;
        clang-tidy)
            command_exists clang-tidy || compgen -G '/usr/bin/clang-tidy-*' >/dev/null
            ;;
        python3-venv)
            python3 -m venv --help >/dev/null 2>&1
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

ensure_venv() {
    if [ ! -d "$HOME/.venv" ]; then
        python3 -m venv ~/.venv
    fi
    export PATH="$HOME/.venv/bin:$PATH"
    # shellcheck source=/dev/null
    source ~/.venv/bin/activate
    pip install --upgrade pip
}

install_conan_if_missing() {
    if command_exists conan; then
        return 0
    fi

    if [ -x "$HOME/.venv/bin/conan" ]; then
        export PATH="$HOME/.venv/bin:$PATH"
        return 0
    fi

    ensure_venv
    pip install conan
    command_exists conan
}

install_gcovr_if_missing() {
    if command_exists gcovr; then
        return 0
    fi

    pip3 install gcovr
    command_exists gcovr
}

install_cmake_format_if_missing() {
    if command_exists cmake-format; then
        return 0
    fi

    pip3 install cmakelang
    command_exists cmake-format
}

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

ensure_single_profile_line() {
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
        python3-venv) echo "python3-venv" ;;
        pip3) echo "python3-pip" ;;
        build-essential) echo "build-essential" ;;
        valgrind) echo "valgrind" ;;
        cppcheck) echo "cppcheck" ;;
        mold) echo "mold" ;;
        graphviz) echo "graphviz" ;;
        feh) echo "feh" ;;
        tmux) echo "tmux" ;;
        gnuplot) echo "gnuplot" ;;
        clang-format) echo "clang-format" ;;
        clang-tidy) echo "clang-tidy" ;;
        rg) echo "ripgrep" ;;
        hexdump) echo "bsdextrautils" ;;
        apt-get) echo "apt" ;;
        sudo) echo "sudo" ;;
        batcat) echo "bat" ;;
        conan|gcovr|cmake-format) echo "" ;;
        *) echo "$cmd" ;;
    esac
}

version_ge() {
    local have="$1"
    local want="$2"
    [ "$(printf '%s\n' "$want" "$have" | sort -V | head -n1)" = "$want" ]
}

extract_first_version() {
    local text="$1"
    echo "$text" | grep -oE '[0-9]+([.][0-9]+)+' | head -n1
}

extract_major() {
    local ver="$1"
    echo "$ver" | cut -d. -f1
}

install_missing_special_tool() {
    local cmd="$1"
    case "$cmd" in
        conan)
            install_conan_if_missing
            ;;
        gcovr)
            install_gcovr_if_missing
            ;;
        cmake-format)
            install_cmake_format_if_missing
            ;;
        *)
            command_exists "$cmd"
            ;;
    esac
}

ensure_tools_for_option() {
    local opt="$1"
    local -a tool_specs
    local -a missing_required missing_recommended missing_optional
    local -a missing_required_apt missing_recommended_apt missing_optional_apt
    local -a missing_required_special missing_recommended_special missing_optional_special
    local -a apt_to_install special_to_install
    local spec cmd requirement pkg reply status display_cmd
    local validate_only=0
    local use_noninteractive_defaults=0
    local compat_failures=0
    local validation_failed=0

    if [ "$opt" = "toolchain" ] || [ "$opt" = "toolchain_required" ]; then
        use_noninteractive_defaults=1
    fi

    case "$opt" in
        "release"|"debug")
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
                "cmake:required" "make:required" "conan:required" "ninja:required"
                "python3:required" "pip3:required" "valgrind:required"
                "hexdump:required" "graphviz:required"
                "cppcheck:recommended" "mold:recommended" "rg:recommended"
                "cmake-format:optional" "clang-format:optional" "clang-tidy:optional" "gdb:optional" "tmux:optional" "feh:optional" "gnuplot:optional"
            )
            ;;
        "toolchain_required")
            tool_specs=(
                "sudo:required" "apt-get:required"
                "gcc:required" "g++:required" "cmake:required" "make:required"
                "ninja:required" "build-essential:required"
                "python3:required" "python3-venv:required" "pip3:required"
                "mold:required" "valgrind:required"
                "graphviz:required" "hexdump:required" "conan:required"
                "clang-format:optional"
            )
            ;;
        "toolchain_all")
            tool_specs=(
                "sudo:required" "apt-get:required"
                "git:required" "gcc:required" "g++:required"
                "cmake:required" "make:required" "conan:required"
                "python3:required" "pip3:required" "valgrind:required"
                "hexdump:required" "graphviz:required"
                "cppcheck:required" "gdb:required" "ninja:required" "mold:required" "cmake-format:required" "clang-format:required" "clang-tidy:required" "rg:required"
                "tmux:required" "feh:required" "gnuplot:required"
            )
            ;;
        "validate")
            tool_specs=(
                "git:required" "gcc:required" "g++:required"
                "cmake:required" "make:required" "conan:required" "ninja:required"
                "python3:required" "pip3:required" "valgrind:required"
                "hexdump:required" "graphviz:required"
                "cppcheck:recommended" "mold:recommended" "rg:recommended"
                "cmake-format:optional" "clang-format:optional" "clang-tidy:optional" "gdb:optional" "tmux:optional" "feh:optional" "gnuplot:optional"
            )
            validate_only=1
            ;;
        "batsyntax")
            tool_specs=("batcat:required")
            ;;
        "bashrc"|"vimsyntax"|"quit"|"help"|"--help"|"-h")
            tool_specs=()
            ;;
        *)
            tool_specs=()
            ;;
    esac

    if [ "$validate_only" -eq 1 ]; then
        echo "-- Validation status:"
        printf "%-16s | %-12s | %-10s\n" "tool" "requirement" "status"
        printf -- "-----------------+--------------+-----------\n"
    fi

    for spec in "${tool_specs[@]}"; do
        cmd=${spec%%:*}
        requirement=${spec##*:}
        display_cmd="$cmd"
        if [ "$cmd" = "batcat" ]; then
            display_cmd="bat/batcat"
        fi

        if ! tool_installed "$cmd"; then
            status="missing"
            pkg=$(cmd_to_apt_package "$cmd")

            case "$requirement" in
                required)
                    missing_required+=("$cmd")
                    if [ -n "$pkg" ] && append_unique "$pkg" "${missing_required_apt[@]}"; then
                        missing_required_apt+=("$pkg")
                    elif [ -z "$pkg" ] && append_unique "$cmd" "${missing_required_special[@]}"; then
                        missing_required_special+=("$cmd")
                    fi
                    ;;
                recommended)
                    missing_recommended+=("$cmd")
                    if [ -n "$pkg" ] && append_unique "$pkg" "${missing_recommended_apt[@]}"; then
                        missing_recommended_apt+=("$pkg")
                    elif [ -z "$pkg" ] && append_unique "$cmd" "${missing_recommended_special[@]}"; then
                        missing_recommended_special+=("$cmd")
                    fi
                    ;;
                optional)
                    missing_optional+=("$cmd")
                    if [ -n "$pkg" ] && append_unique "$pkg" "${missing_optional_apt[@]}"; then
                        missing_optional_apt+=("$pkg")
                    elif [ -z "$pkg" ] && append_unique "$cmd" "${missing_optional_special[@]}"; then
                        missing_optional_special+=("$cmd")
                    fi
                    ;;
            esac
        else
            status="installed"
        fi

        if [ "$validate_only" -eq 1 ]; then
            printf "%-16s | %-12s | %-10s\n" "$display_cmd" "$requirement" "$status"
        fi
    done

    if [ "$validate_only" -eq 1 ]; then
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
            if [ -n "$gcc_ver" ] && version_ge "$gcc_ver" "13"; then
                compat_status="ok"
            else
                compat_status="fail"
                compat_failures=$((compat_failures + 1))
            fi
            printf "%-28s | %-14s | %-8s | %-18s\n" "gcc version" "${gcc_ver:-unknown}" "$compat_status" ">= 13"
        else
            compat_status="fail"
            compat_failures=$((compat_failures + 1))
            printf "%-28s | %-14s | %-8s | %-18s\n" "gcc version" "missing" "$compat_status" ">= 13"
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
    fi

    if [ ${#missing_required[@]} -gt 0 ]; then
        echo "-- Installing missing REQUIRED tools for '$opt': ${missing_required[*]}"
        for pkg in "${missing_required_apt[@]}"; do
            if append_unique "$pkg" "${apt_to_install[@]}"; then
                apt_to_install+=("$pkg")
            fi
        done
        for cmd in "${missing_required_special[@]}"; do
            if append_unique "$cmd" "${special_to_install[@]}"; then
                special_to_install+=("$cmd")
            fi
        done
    fi

    if [ ${#missing_recommended[@]} -gt 0 ]; then
        echo "-- Missing RECOMMENDED tools for '$opt': ${missing_recommended[*]}"
        if [ "$use_noninteractive_defaults" -eq 1 ]; then
            reply=""
            echo "-- Applying default for recommended tools (Y)."
        else
            read -r -p "-- Install missing recommended tools? [Y/n] " reply
        fi
        if [ -z "$reply" ] || [[ "$reply" =~ ^[Yy]$ ]]; then
            for pkg in "${missing_recommended_apt[@]}"; do
                if append_unique "$pkg" "${apt_to_install[@]}"; then
                    apt_to_install+=("$pkg")
                fi
            done
            for cmd in "${missing_recommended_special[@]}"; do
                if append_unique "$cmd" "${special_to_install[@]}"; then
                    special_to_install+=("$cmd")
                fi
            done
        else
            echo "-- Skipping recommended tools install."
        fi
    fi

    if [ ${#missing_optional[@]} -gt 0 ]; then
        echo "-- Missing OPTIONAL tools for '$opt': ${missing_optional[*]}"
        if [ "$use_noninteractive_defaults" -eq 1 ]; then
            reply=""
            echo "-- Applying default for optional tools (N)."
        else
            read -r -p "-- Install missing optional tools? [y/N] " reply
        fi
        if [[ "$reply" =~ ^[Yy]$ ]]; then
            for pkg in "${missing_optional_apt[@]}"; do
                if append_unique "$pkg" "${apt_to_install[@]}"; then
                    apt_to_install+=("$pkg")
                fi
            done
            for cmd in "${missing_optional_special[@]}"; do
                if append_unique "$cmd" "${special_to_install[@]}"; then
                    special_to_install+=("$cmd")
                fi
            done
        else
            echo "-- Skipping optional tools install."
        fi
    fi

    if [ ${#apt_to_install[@]} -gt 0 ]; then
        if ! command_exists sudo || ! command_exists apt-get; then
            echo "Error: cannot auto-install apt packages without sudo and apt-get. Missing packages: ${apt_to_install[*]}"
            exit 1
        fi
        sudo apt-get update
        sudo apt-get -y install "${apt_to_install[@]}"
    fi

    for cmd in "${special_to_install[@]}"; do
        if ! install_missing_special_tool "$cmd"; then
            echo "Error: failed to install $cmd"
            exit 1
        fi
    done

    echo "-- Dependency check complete for option '$opt'."
}

# Install the highest available GCC from a descending version ladder,
# switch system alternatives to it, and verify C++23 support.
install_best_gcc_for_cxx23() {
    local ver priority
    for ver in 20 19 18 17 16 15 14 13; do
        if ! apt-cache show "gcc-$ver" >/dev/null 2>&1 || ! apt-cache show "g++-$ver" >/dev/null 2>&1; then
            continue
        fi

        echo "-- Attempting GCC $ver (highest available candidate)..."
        if ! sudo apt-get -y install "gcc-$ver" "g++-$ver"; then
            echo "-- Install failed for GCC $ver, trying lower version..."
            continue
        fi

        priority=$((100 + ver))
        if [ -x "/usr/bin/gcov-$ver" ]; then
            sudo update-alternatives \
                --install /usr/bin/gcc  gcc  "/usr/bin/gcc-$ver"  "$priority" \
                --slave   /usr/bin/g++  g++  "/usr/bin/g++-$ver" \
                --slave   /usr/bin/gcov gcov "/usr/bin/gcov-$ver"
        else
            sudo update-alternatives \
                --install /usr/bin/gcc  gcc  "/usr/bin/gcc-$ver"  "$priority" \
                --slave   /usr/bin/g++  g++  "/usr/bin/g++-$ver"
        fi

        if check_cxx23; then
            echo "-- Selected GCC $ver with working C++23 support."
            return 0
        fi

        echo "-- GCC $ver installed, but C++23 probe failed. Trying lower version..."
    done

    return 1
}

run_option() {
    local opt="$1"
    if [ "$opt" != "help" ] && [ "$opt" != "--help" ] && [ "$opt" != "-h" ]; then
        ensure_tools_for_option "$opt"
    fi
    case "$opt" in
        "release")
            sed 's/Debug/Release/g' <~/.conan2/profiles/default >~/.conan2/profiles/temp && mv ~/.conan2/profiles/temp ~/.conan2/profiles/default
            conan source $build_folder
            conan install $build_folder -s build_type=Release --build missing
            conan build $build_folder -s build_type=Release --build missing
            ;;
        "debug")
            sed 's/Release/Debug/g' <~/.conan2/profiles/default >~/.conan2/profiles/temp && mv ~/.conan2/profiles/temp ~/.conan2/profiles/default 
            conan source $build_folder
            conan install $build_folder -s build_type=Debug --build missing
            conan build $build_folder -s build_type=Debug --build missing
            ;;
        "toolchain"|"toolchain_all")
            ensure_venv
            if [ ! -f ~/.conan2/profiles/default ]; then conan profile detect; fi
            conan profile show
            echo "-- Verifying C++23 support..."
            if ! check_cxx23; then
                gcc_ver=$(gcc -dumpversion 2>/dev/null | cut -d. -f1)
                echo "-- C++23 not supported (GCC ${gcc_ver:-unknown}). Minimum required: GCC 13."
                echo "-- Attempting GCC ladder install (highest available first)..."
                install_best_gcc_for_cxx23 || { echo "Error: Could not install a C++23-capable GCC (tried versions 20..13). Please install one manually."; exit 1; }
            fi
            echo "-- C++23 OK — g++ $(g++ -dumpversion)"
            if [ "$opt" = "toolchain_all" ]; then
                echo "-- Full toolchain installation complete (required + recommended + optional)."
            fi
            ;;
        "toolchain_required")
            ensure_venv
            echo "-- Minimal CI-like toolchain installation complete."
            ;;
        "validate")
            echo "-- Validation complete."
            ;;
        "conan")
            check_cxx23 || { echo "Error: C++23 not supported by g++ $(g++ -dumpversion 2>/dev/null). Run 'toolchain' first."; exit 1; }
            conan profile detect -f
            sed 's/compiler.cppstd=gnu17/compiler.cppstd=gnu23/g' <~/.conan2/profiles/default >~/.conan2/profiles/temp && mv ~/.conan2/profiles/temp ~/.conan2/profiles/default
            ;;
        "ninja")
            conan_profile="$HOME/.conan2/profiles/default"
            ensure_single_profile_line "$conan_profile" '[conf]' '^[[]conf[]]$'
            ensure_single_profile_line "$conan_profile" 'tools.cmake.cmaketoolchain:generator=Ninja' '^tools[.]cmake[.]cmaketoolchain:generator='
            cat ~/.conan2/profiles/default
            ;;
        "bashrc")
            cd $build_folder
            if [ "${PWD##*/}" != "retractordb" ] ; then echo "Error: Current folder is not retractordb" ; exit ; fi 
            bashrc_file="$HOME/.bashrc"
            desired_path_line="export PATH=\"${PWD}/bin:\$PATH\""
            desired_venv_line="source ~/.venv/bin/activate"
            ensure_single_bashrc_line "$bashrc_file" "$desired_path_line" '^export PATH=".*\/bin:\$PATH"$'
            ensure_single_bashrc_line "$bashrc_file" "$desired_venv_line" '^(source|\.)[[:space:]]+(.*/)?\.venv/bin/activate$'
            echo "-- Last two lines of ~/.bashrc are:"
            tail -n 2 ~/.bashrc
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

            cd $build_folder
            cmake --preset conan-debug -DENABLE_COVERAGE=ON
            cd build/Debug
            find . -name '*.gcda' -delete -o -name '*.gcno' -delete
            ninja clean && ninja
            export PATH="$(pwd)/src/retractor:$(pwd)/src/rdb:$(pwd)/src/qry:$PATH"
            ctest || true
            cd ../..
            rm -f *.gcov
            mkdir -p coverage
            gcovr --root . --filter 'src/' --gcov-executable "$gcov_exec" --gcov-ignore-errors all --exclude '.*\.antlr.*' build/Debug --html-details coverage/coverage.html --xml coverage/coverage.xml --print-summary
            ;;
        "vimsyntax")
            vim_dir="${HOME}/.vim"
            mkdir -p "$vim_dir/syntax" "$vim_dir/ftdetect"
            cp "$build_folder/scripts/.vim/syntax/rql.vim"   "$vim_dir/syntax/"
            cp "$build_folder/scripts/.vim/ftdetect/rql.vim" "$vim_dir/ftdetect/"
            echo "-- RetractorQL vim syntax installed to $vim_dir"
            ;;
        "batsyntax")
            BAT=$(command -v batcat 2>/dev/null || command -v bat 2>/dev/null || true)
            if [ -z "$BAT" ]; then
                echo "Error: neither 'batcat' nor 'bat' found. Install bat first."
                exit 1
            fi
            syntax_src="$build_folder/scripts/sublime/retractorql.sublime-syntax"
            syntax_dir="$("$BAT" --config-dir)/syntaxes"
            mkdir -p "$syntax_dir"
            cp "$syntax_src" "$syntax_dir/"
            "$BAT" cache --build
            echo "-- RetractorQL syntax installed to $syntax_dir"
            ;;
        "quit")
            echo "-- Current conan profile is:"
            cat ~/.conan2/profiles/default
            echo "-- Ok, quit - no action."
            ;;
        "help"|"--help"|"-h")
            echo "Usage: $0 [option ...]"
            echo ""
            echo "Options:"
            echo "  release    - Build in Release mode (conan source, install, build)"
            echo "  debug      - Build in Debug mode (conan source, install, build)"
            echo "  toolchain  - Install build toolchain (gcc, cmake, ninja, conan, etc.)"
            echo "  toolchain_required - Install minimal CI-like required toolchain from config.yml"
            echo "  toolchain_all - Install full toolchain: required + recommended + optional"
            echo "  validate   - Show required/recommended/optional tool status and install state"
            echo "  conan      - Detect conan profile and set C++23 standard"
            echo "  ninja      - Add Ninja generator to conan profile"
            echo "  bashrc     - Add retractordb/bin to PATH in ~/.bashrc"
            echo "  coverage   - Build tests with code coverage enabled"
            echo "  vimsyntax  - Install RetractorQL syntax highlighting for vim"
            echo "  batsyntax  - Install RetractorQL syntax highlighting for bat/batcat"
            echo "  help       - Show this help message"
            echo "  quit       - Show current conan profile and exit"
            echo ""
            echo "Without arguments, runs in interactive mode."
            echo "Multiple options can be passed: $0 conan ninja debug"
            ;;
        *) echo "invalid option: $opt"
              echo "Valid options: release debug conan ninja toolchain toolchain_required toolchain_all validate bashrc coverage vimsyntax batsyntax help quit"
           exit 1
           ;;
    esac
}

if [ $# -gt 0 ]; then
    for arg in "$@"; do
        run_option "$arg"
    done
else
    PS3='-- Pick option, please enter your setup choice: '
    options=("release" "debug" "conan" "ninja" "toolchain" "toolchain_required" "toolchain_all" "validate" "bashrc" "coverage" "vimsyntax" "batsyntax" "help" "quit")
    select opt in "${options[@]}"
    do
        run_option "$opt"
        break
    done
fi
