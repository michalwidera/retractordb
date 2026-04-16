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

run_option() {
    local opt="$1"
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
        "toolchain")
            sudo apt-get update
            sudo apt-get upgrade -y
            sudo apt-get -y install git gcc g++ gdb cmake make ninja-build build-essential python3 python3-pip python3-venv valgrind cppcheck mold graphviz feh tmux gnuplot clang-format ripgrep
            python3 -m venv ~/.venv
            source ~/.venv/bin/activate
            pip3 install --upgrade pip 
            pip3 install conan
            if [ ! -f ~/.conan2/profiles/default ] ; then conan profile detect ; fi
            conan profile show
            ;;
        "conan")
            conan profile detect -f
            sed 's/compiler.cppstd=gnu17/compiler.cppstd=gnu23/g' <~/.conan2/profiles/default >~/.conan2/profiles/temp && mv ~/.conan2/profiles/temp ~/.conan2/profiles/default 
            ;;
        "ninja")
            echo '[conf]' >> ~/.conan2/profiles/default
            echo 'tools.cmake.cmaketoolchain:generator=Ninja' >> ~/.conan2/profiles/default
            cat ~/.conan2/profiles/default
            ;;
        "bashrc")
            cd $build_folder
            if [ "${PWD##*/}" != "retractordb" ] ; then echo "Error: Current folder is not retractordb" ; exit ; fi 
            echo 'export PATH="'${PWD}'/bin:$PATH"' >> ~/.bashrc
            echo 'source ~/.venv/bin/activate' >> ~/.bashrc
            echo "-- Last two lines of ~/.bashrc are:"
            tail -n 2 ~/.bashrc
            ;;
        "coverage")
            gcc_ver=$(gcc -dumpversion | cut -d. -f1)
            gcov_exec="gcov-${gcc_ver}"
            echo "-- GCC $gcc_ver detected, checking coverage tools..."

            # gcov — wymagany w wersji zgodnej z GCC
            if command -v "$gcov_exec" &>/dev/null; then
                gcov_ver=$("$gcov_exec" --version | head -1 | grep -oP '\d+' | head -1)
                if [[ "$gcov_ver" != "$gcc_ver" ]]; then
                    echo "-- gcov version mismatch ($gcov_exec reports $gcov_ver, need $gcc_ver), installing $gcov_exec..."
                    sudo apt-get install -y gcc-${gcc_ver} || { echo "Error: Failed to install $gcov_exec"; exit 1; }
                fi
            else
                echo "-- $gcov_exec not found, installing $gcov_exec..."
                sudo apt-get install -y gcc-${gcc_ver} || { echo "Error: Failed to install $gcov_exec"; exit 1; }
            fi

            # weryfikacja po instalacji
            if ! command -v "$gcov_exec" &>/dev/null; then
                echo "Error: $gcov_exec still not available after install attempt"; exit 1
            fi
            gcov_ver=$("$gcov_exec" --version | head -1 | grep -oP '\d+' | head -1)
            if [[ "$gcov_ver" != "$gcc_ver" ]]; then
                echo "Error: gcov version still mismatched ($gcov_ver != $gcc_ver) after reinstall"; exit 1
            fi
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
            echo "  conan      - Detect conan profile and set C++23 standard"
            echo "  ninja      - Add Ninja generator to conan profile"
            echo "  bashrc     - Add retractordb/bin to PATH in ~/.bashrc"
            echo "  coverage   - Build tests with code coverage enabled"
            echo "  help       - Show this help message"
            echo "  quit       - Show current conan profile and exit"
            echo ""
            echo "Without arguments, runs in interactive mode."
            echo "Multiple options can be passed: $0 conan ninja debug"
            ;;
        *) echo "invalid option: $opt"
           echo "Valid options: release debug conan ninja toolchain bashrc coverage help quit"
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
    options=("release" "debug" "conan" "ninja" "toolchain" "bashrc" "help" "coverage" "quit")
    select opt in "${options[@]}"
    do
        run_option "$opt"
        break
    done
fi
