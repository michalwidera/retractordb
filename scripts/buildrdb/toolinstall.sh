# shellcheck shell=bash

# Modul sprawdzac przez `shellcheck -x scripts/buildrdb.sh` (analiza calego
# programu). Osobno zglasza SC2154/SC2034 na zmiennych dzielonych przez
# zasieg dynamiczny -- to nie jest defekt, tylko brak kontekstu.
#
# Jak zainstalowac to, czego brakuje: instalatory pojedynczych narzedzi oraz
# hurtowa faza instalacji wolana z ensure_tools_for_option.

install_python_venv_support() {
    local py pyver pkg
    py=$(find_python3) || { echo "Error: python3 not found"; exit 1; }
    if ! "$py" -m venv --help >/dev/null 2>&1 || ! "$py" -c 'import ensurepip' >/dev/null 2>&1; then
        pyver=$(echo "$py" | grep -oE '[0-9]+\.[0-9]+' | head -n1)
        if [ -n "$pyver" ]; then
            pkg="python${pyver}-venv"
        else
            pkg="python3-venv"
        fi
        echo "-- Installing $pkg for venv support..."
        sudo apt-get -y install "$pkg"
    fi
}

ensure_venv() {
    local py
    py=$(find_python3) || { echo "Error: python3 not found"; exit 1; }
    install_python_venv_support
    if [ ! -d "$HOME/.venv" ]; then
        echo "-- Creating venv with $py..."
        "$py" -m venv ~/.venv
    fi
    export PATH="$HOME/.venv/bin:$PATH"
    # shellcheck source=/dev/null
    source ~/.venv/bin/activate
    pip install --upgrade pip
}

add_venv_to_bashrc() {
    local bashrc="$HOME/.bashrc"
    local desired="source ~/.venv/bin/activate"
    local regex='^(source|\.)[[:space:]]+(.*/)?\.venv/bin/activate$'
    if grep -qF "$desired" "$bashrc" 2>/dev/null; then
        echo "-- venv activation already in ~/.bashrc, skipping"
        return 0
    fi
    if grep -qE "$regex" "$bashrc" 2>/dev/null; then
        echo "-- Updating venv activation line in ~/.bashrc"
    else
        echo "-- Adding venv activation to ~/.bashrc"
    fi
    ensure_single_bashrc_line "$bashrc" "$desired" "$regex"
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

# CMake z apt jest tak stary (albo tak nowy), jak akurat wypadlo dystrybucji.
# Przypieta wersja ladowana jest do ~/.venv, ktore i tak jest aktywowane przez
# ~/.bashrc i przez CI, wiec `cmake` z PATH staje sie ta sama wersja, ktorej uzywa
# conan. Systemowego cmake to nie rusza.
install_pinned_cmake_if_needed() {
    ensure_venv
    # --only-binary: PyPI ma dla cmake takze sdist, a jego budowa to kompilacja
    # calego CMake ze zrodel. Na platformie bez kola ma sie nie udac od razu,
    # zamiast zajmowac CI na godziny. Kola sa dla x86_64 i aarch64, wiec job ARM
    # jest pokryty.
    if ! pip install --only-binary=:all: "cmake>=$RDB_CMAKE_MIN_VERSION"; then
        echo "-- WARNING: brak koła cmake>=$RDB_CMAKE_MIN_VERSION dla tej platformy."
        echo "-- WARNING: build nadal zadziala — conan wciaga wlasnego cmake — ale"
        echo "--          'cmake' wolane wprost z PATH bedzie starsza wersja."
        return 0
    fi
    if ! tool_installed cmake-pinned; then
        echo "-- WARNING: 'cmake' z PATH jest nadal starszy niz $RDB_CMAKE_MIN_VERSION."
        echo "--          Sprawdz, czy ~/.venv jest aktywowane przed systemowym cmake."
    fi
    return 0
}

install_cmake_format_if_missing() {
    if command_exists cmake-format; then
        return 0
    fi

    pip3 install cmakelang
    command_exists cmake-format
}

install_missing_special_tool() {
    local cmd="$1"
    case "$cmd" in
        python3-venv)
            install_python_venv_support
            ;;
        conan)
            install_conan_if_missing
            ;;
        gcovr)
            install_gcovr_if_missing
            ;;
        cmake-format)
            install_cmake_format_if_missing
            ;;
        cmake-pinned)
            install_pinned_cmake_if_needed
            ;;
        *)
            command_exists "$cmd"
            ;;
    esac
}

# Install the highest available GCC from a descending version ladder,
# switch system alternatives to it, and verify C++23 support.
install_best_gcc_for_cxx23() {
    local ver priority
    for ver in 20 19 18 17 16 15 14; do
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

# Verify C++23 support (probe) and, if missing, install the best available GCC
# via the ladder above. Exits the script on failure.
ensure_cxx23_gcc() {
    local gcc_ver
    echo "-- Verifying C++23 support..."
    if ! check_cxx23; then
        gcc_ver=$(gcc -dumpversion 2>/dev/null | cut -d. -f1)
        echo "-- C++23 not supported (GCC ${gcc_ver:-unknown}). Minimum required: GCC 14."
        echo "-- Attempting GCC ladder install (highest available first)..."
        install_best_gcc_for_cxx23 || { echo "Error: Could not install a C++23-capable GCC (tried versions 20..14). Please install one manually."; exit 1; }
    fi
    echo "-- C++23 OK — g++ $(g++ -dumpversion)"
}

# Faza instalacyjna: zamienia listy brakow policzone przez ensure_tools_for_option
# na jedno wywolanie apt i na instalatory specjalne. Czyta `missing_*` i
# `use_noninteractive_defaults` z zasiegu wywolujacego.
install_missing_tools() {
    local opt="$1"

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
        # Acquire::Retries: lustra potrafia oddac 503 na POJEDYNCZYM pliku, a bez
        # ponowien wywraca to caly job. Na CI ARM (us-east-1.ec2.ports.ubuntu.com)
        # zdarza sie to okresowo na fonts-liberation, ciagnietym jako zaleznosc
        # graphviza z listy `toolchain_required`. apt ponawia samo POBRANIE pozycji,
        # wiec to jest ta warstwa, na ktorej awaria lustra ma byc obsluzona.
        sudo apt-get -o Acquire::Retries=5 update
        sudo apt-get -o Acquire::Retries=5 -y install "${apt_to_install[@]}"
    fi

    for cmd in "${special_to_install[@]}"; do
        if ! install_missing_special_tool "$cmd"; then
            echo "Error: failed to install $cmd"
            exit 1
        fi
    done

    echo "-- Dependency check complete for option '$opt'."
}
