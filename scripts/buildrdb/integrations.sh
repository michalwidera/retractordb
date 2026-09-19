# shellcheck shell=bash

attach_knowledge_index() {
    local repository_url="git@github.com:michalwidera/knowledge-index.git"
    local knowledge_index_dir
    local skill_link="$rdb_source_dir/.agents/skills/retractordb-system"
    local resolved_skill_dir

    knowledge_index_dir="$(dirname "$rdb_source_dir")/knowledge-index"

    if [ -e "$knowledge_index_dir" ]; then
        if [ ! -d "$knowledge_index_dir" ] || ! git -C "$knowledge_index_dir" rev-parse --is-inside-work-tree >/dev/null 2>&1; then
            echo "Error: $knowledge_index_dir exists but is not a Git repository."
            return 1
        fi
        echo "-- Knowledge index already exists: $knowledge_index_dir"
    else
        echo "-- Cloning knowledge index into $knowledge_index_dir"
        git clone "$repository_url" "$knowledge_index_dir"
    fi

    if [ ! -L "$skill_link" ]; then
        echo "Error: expected tracked symbolic link is missing: $skill_link"
        return 1
    fi

    if ! resolved_skill_dir=$(cd "$skill_link" 2>/dev/null && pwd -P); then
        echo "Error: symbolic link does not resolve: $skill_link"
        return 1
    fi

    # Porownanie na sciezkach FIZYCZNYCH, komunikaty na logicznych.
    #
    # `resolved_skill_dir` powstaje przez `pwd -P`, wiec jest fizyczna zawsze;
    # `knowledge_index_dir` idzie z `dirname` i zostaje taka, jaka widzi uzytkownik.
    # Na Linuksie to zwykle to samo, ale na macOS /var i /tmp sa dowiazaniami do
    # /private/..., wiec poprawnie podpiety indeks wygladal jak niezgodny:
    # "/private/var/..." kontra "/var/...". Rozdzielamy wiec te dwie role zamiast
    # normalizowac jedna z nich - komunikat ma pokazywac sciezke, ktora czytelnik
    # rozpozna, a porownanie ma byc odporne na dowiazania.
    local knowledge_index_real
    knowledge_index_real=$(cd "$knowledge_index_dir" 2>/dev/null && pwd -P) || knowledge_index_real="$knowledge_index_dir"

    if [ "$resolved_skill_dir" != "$knowledge_index_real" ] || [ ! -f "$resolved_skill_dir/SKILL.md" ]; then
        echo "Error: $skill_link does not point to a valid knowledge index at $knowledge_index_dir"
        return 1
    fi

    # Sciezka LOGICZNA, tak jak w komunikacie wyzej: rownosc z fizyczna zostala
    # wlasnie sprawdzona, a czytelnik ma zobaczyc te sama postac, ktorej sam uzywa.
    echo "-- RetractorDB knowledge index attached: $knowledge_index_dir"
}
run_integration_option() {
    local opt="$1"
    case "$opt" in
        "attach_knowledge")
            attach_knowledge_index
            ;;
        "vimsyntax")
            vim_dir="${HOME}/.vim"
            mkdir -p "$vim_dir/syntax" "$vim_dir/ftdetect"
            cp "$rdb_source_dir/scripts/.vim/syntax/rql.vim"   "$vim_dir/syntax/"
            cp "$rdb_source_dir/scripts/.vim/ftdetect/rql.vim" "$vim_dir/ftdetect/"
            echo "-- RetractorQL vim syntax installed to $vim_dir"
            ;;
        "batsyntax")
            BAT=$(command -v batcat 2>/dev/null || command -v bat 2>/dev/null || true)
            if [ -z "$BAT" ]; then
                echo "Error: neither 'batcat' nor 'bat' found. Install bat first."
                exit 1
            fi
            syntax_src="$rdb_source_dir/scripts/sublime/retractorql.sublime-syntax"
            syntax_dir="$("$BAT" --config-dir)/syntaxes"
            mkdir -p "$syntax_dir"
            cp "$syntax_src" "$syntax_dir/"
            "$BAT" cache --build
            echo "-- RetractorQL syntax installed to $syntax_dir"
            ;;
    esac
}
