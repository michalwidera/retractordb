# shellcheck shell=bash

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

run_profile_option() {
    local opt="$1"
    case "$opt" in
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
            cd "$rdb_source_dir"
            bashrc_file="$HOME/.bashrc"
            # Binaria instalują się do ~/.local/bin (prefiks ustawiany w CMakeLists).
            # Tworzymy katalog, by wpis PATH był poprawny nawet przed pierwszym 'ninja
            # install'. Regex '.*/bin' przy podmianie usuwa też stare wpisy <repo>/bin.
            mkdir -p "$HOME/.local/bin"
            desired_path_line="export PATH=\"\$HOME/.local/bin:\$PATH\""
            desired_venv_line="source ~/.venv/bin/activate"
            # Uwaga awk: literalny '$' w dynamicznym regexie wymaga '\\$' (pojedyncze
            # '\$' awk -v zamienia na kotwicę końca linii → dawny wzorzec nigdy nie
            # pasował i wpisy PATH się duplikowały). Teraz podmienia stare <repo>/bin.
            ensure_single_bashrc_line "$bashrc_file" "$desired_path_line" '^export PATH=".*/bin:\\$PATH"$'
            ensure_single_bashrc_line "$bashrc_file" "$desired_venv_line" '^(source|\.)[[:space:]]+(.*/)?\.venv/bin/activate$'
            echo "-- Last two lines of ~/.bashrc are:"
            tail -n 2 ~/.bashrc
            ;;
    esac
}
