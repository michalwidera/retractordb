# shellcheck shell=bash

# Modul sprawdzac przez `shellcheck -x scripts/buildrdb.sh` (analiza calego
# programu). Osobno zglasza SC2154/SC2034 na zmiennych dzielonych przez
# zasieg dynamiczny -- to nie jest defekt, tylko brak kontekstu.
#
# Orkiestracja sprawdzania zaleznosci przed wykonaniem opcji.
#
# KONTRAKT ZMIENNYCH: tablice brakow i liczniki nizej sa zwyklymi zmiennymi
# lokalnymi tej funkcji, ale czytaja i pisza je rowniez tool_specs_for_option
# (toolmatrix.sh), report_validation_status (toolreport.sh) i
# install_missing_tools (toolinstall.sh) -- bash ma zasieg dynamiczny, wiec
# widza zmienne wywolujacego. Przy zmianie nazwy ktorejkolwiek z nich poprawic
# wszystkie trzy moduly.
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

    tool_specs_for_option "$opt"

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
        report_validation_status || return 1
        return 0
    fi

    install_missing_tools "$opt"
}

run_dependency_option() {
    local opt="$1"
    case "$opt" in
        "toolchain"|"toolchain_all")
            ensure_venv
            add_venv_to_bashrc
            if [ ! -f ~/.conan2/profiles/default ]; then conan profile detect; fi
            conan profile show
            ensure_cxx23_gcc
            if [ "$opt" = "toolchain_all" ]; then
                echo "-- Full toolchain installation complete (required + recommended + optional)."
            fi
            ;;
        "toolchain_required")
            ensure_venv
            add_venv_to_bashrc
            ensure_cxx23_gcc
            echo "-- Minimal CI-like toolchain installation complete."
            ;;
        "validate")
            echo "-- Validation complete."
            ;;
    esac
}
