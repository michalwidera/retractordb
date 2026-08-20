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

    if [ "$resolved_skill_dir" != "$knowledge_index_dir" ] || [ ! -f "$resolved_skill_dir/SKILL.md" ]; then
        echo "Error: $skill_link does not point to a valid knowledge index at $knowledge_index_dir"
        return 1
    fi

    echo "-- RetractorDB knowledge index attached: $resolved_skill_dir"
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
