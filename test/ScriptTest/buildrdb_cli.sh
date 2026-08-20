#!/usr/bin/env bash

set -euo pipefail

source_root="${1:?usage: buildrdb_cli.sh SOURCE_ROOT}"
buildrdb="$source_root/scripts/buildrdb.sh"
test_root=$(mktemp -d)
trap 'rm -rf "$test_root"' EXIT

fail() {
  echo "FAIL: $*" >&2
  exit 1
}

assert_contains() {
  local output="$1"
  local expected="$2"
  [[ "$output" == *"$expected"* ]] || fail "missing output: $expected"
}

make_home() {
  local home_dir="$1"
  mkdir -p "$home_dir"
  : > "$home_dir/.bashrc"
}

copy_launcher() {
  local destination="$1"
  mkdir -p "$destination/scripts"
  cp "$buildrdb" "$destination/scripts/buildrdb.sh"
  # Katalog modulow jest obowiazkowy - launcher sam z siebie nic nie umie.
  cp -R "$source_root/scripts/buildrdb" "$destination/scripts/buildrdb"
  : > "$destination/CMakeLists.txt"
  mkdir -p "$destination/src" "$destination/.agents/skills"
  ln -s ../../../knowledge-index "$destination/.agents/skills/retractordb-system"
}

home_dir="$test_root/home"
make_home "$home_dir"

output=$(HOME="$home_dir" "$buildrdb" --help 2>&1)
assert_contains "$output" "Usage:"
assert_contains "$output" "Multiple options can be passed"
assert_contains "$output" "attach_knowledge"

if HOME="$home_dir" "$buildrdb" invalid-option >"$test_root/invalid.log" 2>&1; then
  fail "invalid option succeeded"
fi
output=$(<"$test_root/invalid.log")
assert_contains "$output" "invalid option: invalid-option"
assert_contains "$output" "Valid options:"

# Numer pozycji w menu 'select' zalezy od kolejnosci opcji, wiec odczytujemy go
# z samego menu zamiast wpisywac na sztywno: dopisanie opcji przesuwa numery i
# test wybieralby wtedy co innego, nie zglaszajac tego jako bledu.
menu=$(printf '0\n' | HOME="$home_dir" "$buildrdb" 2>&1 || true)
help_index=$(printf '%s\n' "$menu" | grep -oE '[0-9]+\) help([^_[:alnum:]]|$)' | grep -oE '^[0-9]+' | head -n1)
[[ -n "$help_index" ]] || fail "no 'help' entry in the interactive menu"

output=$(printf '%s\n' "$help_index" | HOME="$home_dir" "$buildrdb" 2>&1)
assert_contains "$output" "Usage:"
assert_contains "$output" "Without arguments, runs in interactive mode."

existing_parent="$test_root/existing"
existing_source="$existing_parent/retractordb"
copy_launcher "$existing_source"
mkdir -p "$existing_parent/knowledge-index"
git -C "$existing_parent/knowledge-index" init -q
: > "$existing_parent/knowledge-index/SKILL.md"
output=$(HOME="$home_dir" "$existing_source/scripts/buildrdb.sh" attach_knowledge 2>&1)
assert_contains "$output" "Knowledge index already exists: $existing_parent/knowledge-index"
assert_contains "$output" "RetractorDB knowledge index attached: $existing_parent/knowledge-index"

clone_parent="$test_root/clone"
clone_source="$clone_parent/retractordb"
copy_launcher "$clone_source"
stub_dir="$test_root/clone-stubs"
mkdir -p "$stub_dir"
cat > "$stub_dir/git" <<'EOF'
#!/usr/bin/env bash
set -euo pipefail
printf '%s\n' "$*" >> "$RDB_TEST_GIT_LOG"
if [[ "$1" == "clone" ]]; then
  mkdir -p "$3/.git"
  : > "$3/SKILL.md"
  exit 0
fi
exit 1
EOF
chmod +x "$stub_dir/git"
git_log="$test_root/git.log"
output=$(HOME="$home_dir" PATH="$stub_dir:$PATH" RDB_TEST_GIT_LOG="$git_log" \
  "$clone_source/scripts/buildrdb.sh" attach_knowledge 2>&1)
assert_contains "$output" "Cloning knowledge index into $clone_parent/knowledge-index"
assert_contains "$output" "RetractorDB knowledge index attached: $clone_parent/knowledge-index"
assert_contains "$(<"$git_log")" "clone git@github.com:michalwidera/knowledge-index.git $clone_parent/knowledge-index"

state_parent="$test_root/state"
state_source="$state_parent/retractordb"
copy_launcher "$state_source"
state_home="$test_root/state-home"
make_home "$state_home"
mkdir -p "$state_home/.conan2/profiles"
printf '%s\n' 'build_type=Release' > "$state_home/.conan2/profiles/default"
state_stubs="$test_root/state-stubs"
mkdir -p "$state_stubs"
for command_name in gcc g++ cmake ninja make; do
  cat > "$state_stubs/$command_name" <<'EOF'
#!/usr/bin/env bash
exit 0
EOF
  chmod +x "$state_stubs/$command_name"
done
cat > "$state_stubs/conan" <<'EOF'
#!/usr/bin/env bash
set -euo pipefail
printf 'jobs=%s mold=%s args=%s\n' \
  "${RDB_BUILD_JOBS:-unset}" "${RDB_USE_MOLD:-unset}" "$*" >> "$RDB_TEST_STATE_LOG"
EOF
chmod +x "$state_stubs/conan"
state_log="$test_root/state.log"
HOME="$state_home" PATH="$state_stubs:$PATH" RDB_TEST_STATE_LOG="$state_log" \
  "$state_source/scripts/buildrdb.sh" lowmem nomold debug >"$test_root/state-output.log" 2>&1
assert_contains "$(<"$test_root/state-output.log")" "Manual override ENABLED"
assert_contains "$(<"$test_root/state-output.log")" "mold linker DISABLED"
while IFS= read -r line; do
  assert_contains "$line" "jobs=2 mold=OFF"
done < "$state_log"
[[ $(wc -l < "$state_log") -eq 3 ]] || fail "expected three conan calls"

echo "buildrdb CLI tests passed"
