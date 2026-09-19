#!/usr/bin/env bash
# Przygotowanie wydania: sprawdzenia wstepne, budowa paczek w Dockerze (x86_64 z .deb,
# arm64) i instrukcja publikacji na GitHub. Skrypt niczego nie publikuje ani nie
# taguje - te kroki wypisuje dla czlowieka. --check konczy po sprawdzeniach.
set -euo pipefail

usage() { echo "Usage: $0 [--check]" >&2; exit 2; }
check_only=0
case "${1:-}" in
  "") ;;
  --check) check_only=1 ;;
  *) usage ;;
esac

repo_dir=$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)
cd "$repo_dir"

failures=0
pass() { printf '  [ OK ] %s\n' "$*"; }
warn() { printf '  [WARN] %s\n' "$*"; }
fail() { printf '  [FAIL] %s\n' "$*"; failures=$((failures + 1)); }

version=$(<VERSION)
tag="v$version"
echo "Preflight for release $tag"

missing_tools=0
for tool in git docker gh sha256sum; do
  if ! command -v "$tool" >/dev/null; then
    fail "$tool is not installed"
    missing_tools=1
  fi
done
((missing_tools == 0)) || { echo "Install the missing tools and run again." >&2; exit 1; }

if docker info >/dev/null 2>&1; then
  pass "Docker daemon is running"
  # Paczka arm64 powstaje pod emulacja qemu (binfmt) - bez niej build ARM nie ruszy.
  if [[ $(docker run --rm --platform linux/arm64 debian:trixie-slim uname -m 2>/dev/null) == aarch64 ]]; then
    pass "Docker runs linux/arm64 containers"
  else
    fail "Docker cannot run linux/arm64 containers (qemu binfmt emulation missing)"
  fi
else
  fail "Docker daemon is not running"
fi

if gh auth status >/dev/null 2>&1; then
  pass "gh is authenticated"
else
  fail "gh is not authenticated (gh auth login)"
fi

if [[ $version =~ ^[0-9]+\.[0-9]+\.[0-9]+$ ]]; then
  pass "VERSION is $version"
else
  fail "VERSION '$version' is not x.y.z"
fi

branch=$(git symbolic-ref --short -q HEAD || true)
if [[ $branch == master ]]; then
  pass "On branch master"
else
  fail "Not on master (${branch:-detached HEAD})"
fi

if [[ -z $(git status --porcelain=v1 --untracked-files=all) ]]; then
  pass "Working tree is clean"
else
  fail "Working tree has uncommitted or untracked files"
fi

head=$(git rev-parse HEAD)
if git fetch --quiet origin master && [[ $(git rev-parse origin/master) == "$head" ]]; then
  pass "HEAD ${head:0:8} is origin/master"
else
  fail "HEAD ${head:0:8} is not origin/master (push master or pull first)"
fi

previous=$(git tag -l 'v[0-9]*.[0-9]*.[0-9]*' | grep -vxF "$tag" | sort -V | tail -1 || true)
if [[ -z $previous || $(printf '%s\n%s\n' "${previous#v}" "$version" | sort -V | tail -1) == "$version" && ${previous#v} != "$version" ]]; then
  pass "VERSION $version is newer than ${previous:-any tag}"
else
  fail "VERSION $version is not newer than $previous - bump VERSION"
fi

if git rev-parse -q --verify "refs/tags/$tag" >/dev/null; then
  if [[ $(git rev-parse "$tag^{commit}") == "$head" ]]; then
    pass "Local tag $tag points at HEAD"
  else
    fail "Local tag $tag points at another commit"
  fi
else
  pass "Local tag $tag does not exist yet"
fi

remote_tag=$(git ls-remote --tags origin "refs/tags/$tag" "refs/tags/$tag^{}" | tail -1 | cut -f1)
if [[ -z $remote_tag ]]; then
  pass "Tag $tag is not on origin yet"
elif [[ $remote_tag == "$head" ]]; then
  pass "Tag $tag on origin points at HEAD"
else
  fail "Tag $tag on origin points at another commit"
fi

if gh release view "$tag" >/dev/null 2>&1; then
  fail "GitHub release $tag already exists"
else
  pass "GitHub release $tag does not exist yet"
fi

slug=$(gh repo view --json nameWithOwner -q .nameWithOwner 2>/dev/null || true)
ci_state=$(gh api "repos/$slug/commits/$head/status" -q .state 2>/dev/null || true)
case "$ci_state" in
  success) pass "CI status of HEAD: success" ;;
  failure | error) fail "CI status of HEAD: $ci_state" ;;
  *) warn "CI status of HEAD: ${ci_state:-unknown} - check CircleCI before publishing" ;;
esac

free_kb=$(df -Pk "$repo_dir" | awk 'NR == 2 {print $4}')
if ((free_kb < 10 * 1024 * 1024)); then
  warn "Less than 10 GB free on the build disk"
else
  pass "Free disk space: $((free_kb / 1024 / 1024)) GB"
fi

if ((failures > 0)); then
  echo "$failures check(s) failed - fix them and run again." >&2
  exit 1
fi
((check_only == 0)) || exit 0

# Kolejno, nie rownolegle: dwa buildy naraz nie mieszcza sie w RAM.
"$repo_dir/scripts/release_package/package-portable-x86.sh" --fresh
"$repo_dir/scripts/release_package/package-portable-arm64.sh" --fresh

assets_dir="build/Release-Assets/$tag"
rm -rf -- "$assets_dir"
mkdir -p "$assets_dir"
cp -- "build/X86-Packages/retractordb-$version-Linux.deb" \
  "build/X86-Packages/retractordb-$version-linux-x86_64-portable.tar.gz" \
  "build/ARM64-Packages/retractordb-$version-linux-aarch64-portable.tar.gz" \
  "$assets_dir/"

echo
echo "Release assets in $assets_dir:"
(cd "$assets_dir" && sha256sum -- *)
echo
echo "To publish $tag:"
step=1
if ! git rev-parse -q --verify "refs/tags/$tag" >/dev/null; then
  echo "  $step. git tag $tag"
  step=$((step + 1))
fi
echo "  $step. git push origin $tag"
echo "  $((step + 1)). gh release create $tag --verify-tag --title $tag --generate-notes $assets_dir/*"
echo "  $((step + 2)). scripts/release_package/release-verify.sh $version"
