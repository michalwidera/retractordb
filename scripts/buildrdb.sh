#!/bin/bash
# shellcheck source-path=SCRIPTDIR

# https://zfredenburg.medium.com/force-a-bash-script-to-exit-on-error-ec50b374c98d
set -o errexit

echo "-- Last two lines of ~/.bashrc are:"
tail -n 2 ~/.bashrc

# The source tree is located from this script's own path, not from the name of the
# current directory. Matching directory names ("retractordb", "build", "Release", ...)
# refused to work in a checkout cloned under any other name - which is exactly what a
# second pinned build tree needs.
script_dir=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
rdb_source_dir=$(cd "$script_dir/.." && pwd)

if [ ! -f "$rdb_source_dir/CMakeLists.txt" ] || [ ! -d "$rdb_source_dir/src" ]; then
    echo "Not a RetractorDB source tree: $rdb_source_dir"
    exit 1
fi

echo "-- Note: Current folder is [ ${PWD##*/} ] and will start build in [ $rdb_source_dir ]"

# Modules hold only definitions, so this order does not matter at run time; it
# follows the order of the option groups in run_option below. Cross-module calls
# (e.g. profiles.sh -> check_cxx23 from toolmatrix.sh) resolve because every
# module is sourced before the first option runs.
source "$script_dir/buildrdb/common.sh"
source "$script_dir/buildrdb/toolmatrix.sh"
source "$script_dir/buildrdb/toolinstall.sh"
source "$script_dir/buildrdb/toolreport.sh"
source "$script_dir/buildrdb/dependencies.sh"
source "$script_dir/buildrdb/profiles.sh"
source "$script_dir/buildrdb/builds.sh"
source "$script_dir/buildrdb/research.sh"
source "$script_dir/buildrdb/integrations.sh"

show_help() {
            echo "Usage: $0 [option ...]"
            echo ""
            echo "Options:"
            echo "  release    - Build verified production Release from a pristine Git tree"
            echo "  release-dirty - Build Release from the tree as it is (uncommitted changes allowed); NOT a production release"
            echo "  release-ablation - Select optimizer/probe switches and build an isolated Release variant"
            echo "  debug      - Build in Debug mode (conan source, install, build)"
            echo "  probe      - Build isolated Release-Probe with RDB_BENCH_PROBE=ON; NOT for production"
            echo "  package    - Build DEB/TGZ packages (cpack) and clean staging artifacts"
            echo "  toolchain  - Install build toolchain (gcc, cmake, ninja, conan, etc.)"
            echo "  toolchain_required - Install minimal CI-like required toolchain from config.yml"
            echo "  toolchain_all - Install full toolchain: required + recommended + optional"
            echo "  validate   - Show required/recommended/optional tool status and install state"
            echo "  conan      - Detect conan profile and set C++23 standard"
            echo "  ninja      - Add Ninja generator to conan profile"
            echo "  bashrc     - Add ~/.local/bin to PATH in ~/.bashrc (and create it)"
            echo "  coverage   - Build tests with code coverage enabled"
            echo "  gate_requirements - Install deps of the research gate (JDK 17 + Flink 2.3.0) and verify them"
            echo "  attach_knowledge - Clone and attach the sibling RetractorDB knowledge index"
            echo "  mold       - Enable mold linker for subsequent options (default; e.g. buildrdb.sh mold debug)"
            echo "  nomold     - Disable mold linker for subsequent options (e.g. RPi: buildrdb.sh nomold debug)"
            echo "  lowmem     - Force build parallelism to a fixed 2 jobs, overriding the automatic RAM-aware default (e.g. RPi: buildrdb.sh lowmem release)"
            echo "  nolowmem   - Clear that override; parallelism reverts to the automatic RAM-aware default (always applied) for subsequent options"
            echo "  vimsyntax  - Install RetractorQL syntax highlighting for vim"
            echo "  batsyntax  - Install RetractorQL syntax highlighting for bat/batcat"
            echo "  help       - Show this help message"
            echo "  quit       - Show current conan profile and exit"
            echo ""
            echo "Without arguments, runs in interactive mode."
            echo "Multiple options can be passed: $0 conan ninja debug"
}

run_option() {
    local opt="$1"
    if [ "$opt" != "help" ] && [ "$opt" != "--help" ] && [ "$opt" != "-h" ]; then
        ensure_tools_for_option "$opt"
    fi

    case "$opt" in
        "release"|"release-dirty"|"debug"|"package"|"coverage") run_build_option "$opt" ;;
        "release-ablation"|"probe"|"gate_requirements") run_research_option "$opt" ;;
        "toolchain"|"toolchain_required"|"toolchain_all"|"validate") run_dependency_option "$opt" ;;
        "conan"|"ninja"|"bashrc") run_profile_option "$opt" ;;
        "attach_knowledge"|"vimsyntax"|"batsyntax") run_integration_option "$opt" ;;
        "mold"|"nomold"|"lowmem"|"nolowmem"|"quit") run_common_option "$opt" ;;
        "help"|"--help"|"-h") show_help ;;
        *)
            echo "invalid option: $opt"
            echo "Valid options: release release-dirty release-ablation debug probe package conan ninja toolchain toolchain_required toolchain_all validate bashrc coverage gate_requirements attach_knowledge mold nomold lowmem nolowmem vimsyntax batsyntax help quit"
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
    options=("release" "release-dirty" "release-ablation" "debug" "probe" "package" "conan" "ninja" "toolchain" "toolchain_required" "toolchain_all" "validate" "bashrc" "coverage" "gate_requirements" "attach_knowledge" "mold" "nomold" "lowmem" "nolowmem" "vimsyntax" "batsyntax" "help" "quit")
    select opt in "${options[@]}"
    do
        run_option "$opt"
        break
    done
fi
