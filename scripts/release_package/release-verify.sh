#!/usr/bin/env bash
# Sprawdzenie opublikowanego wydania na GitHub: czy sa wszystkie pliki potrzebne do
# instalacji i czy instalator ze strony (rdbweb install.sh) je przyjmie. Instalator bierze
# SHA-256 z pola digest w API GitHub Releases, nie z osobnych plikow .sha256 - dlatego
# sprawdzany jest digest, a pobrane archiwa przechodza te same kontrole co w instalatorze.
# Uzycie: scripts/release_package/release-verify.sh [wersja]   (domyslnie z pliku VERSION)
set -euo pipefail

repo_dir=$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)
cd "$repo_dir"

version=${1:-$(<VERSION)}
version=${version#v}
tag="v$version"
slug=$(gh repo view --json nameWithOwner -q .nameWithOwner)
tmp=$(mktemp -d)
trap 'rm -rf -- "$tmp"' EXIT

failures=0
pass() { printf '  [ OK ] %s\n' "$*"; }
warn() { printf '  [WARN] %s\n' "$*"; }
fail() { printf '  [FAIL] %s\n' "$*"; failures=$((failures + 1)); }

echo "Verifying GitHub release $tag of $slug"
if ! gh api "repos/$slug/releases/tags/$tag" >"$tmp/release.json" 2>/dev/null; then
  fail "No published release $tag (drafts are not visible to the installer either)"
  exit 1
fi
gh api "repos/$slug/releases?per_page=100" >"$tmp/releases.json"

remote_tag=$(git ls-remote --tags origin "refs/tags/$tag" "refs/tags/$tag^{}" | tail -1 | cut -f1)
if [[ -z $remote_tag ]]; then
  fail "Tag $tag is not on origin"
elif ! git rev-parse -q --verify "refs/tags/$tag" >/dev/null; then
  warn "Tag $tag is on origin (${remote_tag:0:8}) but not in this clone"
elif [[ $(git rev-parse "$tag^{commit}") == "$remote_tag" ]]; then
  pass "Tag $tag on origin matches the local tag (${remote_tag:0:8})"
else
  fail "Tag $tag on origin (${remote_tag:0:8}) differs from the local tag"
fi

python3 - "$tmp" "$slug" "$version" "$repo_dir/build/Release-Assets/$tag" <<'PY' || failures=$((failures + 1))
import hashlib, json, os, re, struct, subprocess, sys, tarfile, urllib.request

tmp, slug, version, local_dir = sys.argv[1:5]
tag = f'v{version}'
failures = 0

def ok(message):
    print(f'  [ OK ] {message}')

def warn(message):
    print(f'  [WARN] {message}')

def fail(message):
    global failures
    failures += 1
    print(f'  [FAIL] {message}')

def sha256(path):
    with open(path, 'rb') as source:
        return hashlib.sha256(source.read()).hexdigest()

# Te same reguly co w install.sh: dokladny zestaw plikow i katalogow, ELF wlasciwej architektury.
def portable_problem(path, machine):
    required = {'bin/xretractor', 'bin/xqry', 'bin/xtrdb'}
    files = required | {'share/doc/retractordb/LICENSE', 'share/retractordb/retractor.toml'}
    dirs = {'bin', 'share', 'share/doc', 'share/doc/retractordb', 'share/retractordb'}
    with tarfile.open(path, 'r:gz') as archive:
        members = archive.getmembers()
        if {member.name for member in members if member.isfile()} != files:
            return 'unexpected files in the archive'
        for member in members:
            if not (member.isfile() or member.isdir()) or (member.isdir() and member.name.rstrip('/') not in dirs):
                return f'unexpected archive member {member.name}'
        for name in sorted(required):
            with archive.extractfile(name) as source:
                header = source.read(20)
            if header[:4] != b'\x7fELF' or struct.unpack_from('<H', header, 18)[0] != machine:
                return f'{name} has the wrong ELF architecture'
    return None

def deb_problem(path):
    fields = subprocess.run(['dpkg-deb', '-f', path, 'Package', 'Version', 'Architecture'],
                            check=True, capture_output=True, text=True).stdout.split('\n')
    if fields[:3] != [f'Package: retractordb', f'Version: {version}', 'Architecture: amd64']:
        return 'unexpected control fields: ' + ', '.join(fields[:3])
    return None

with open(os.path.join(tmp, 'release.json'), encoding='utf-8') as source:
    release = json.load(source)
if release.get('draft') or release.get('prerelease'):
    fail('Release is a draft or prerelease - the installer skips it')
else:
    ok('Release is published (not draft, not prerelease)')

machines = {'x86_64': 62, 'aarch64': 183}
assets = {asset.get('name'): asset for asset in release.get('assets', [])}
expected = [f'retractordb-{version}-Linux.deb']
expected += [f'retractordb-{version}-linux-{arch}-portable.tar.gz' for arch in machines]
for name in expected:
    asset = assets.get(name)
    if asset is None:
        fail(f'{name}: missing')
        continue
    digest = asset.get('digest') or ''
    url = asset.get('browser_download_url', '')
    if asset.get('state') != 'uploaded' or not url.startswith(f'https://github.com/{slug}/releases/download/'):
        fail(f'{name}: state {asset.get("state")}, url {url}')
        continue
    if not re.fullmatch(r'sha256:[0-9a-fA-F]{64}', digest):
        fail(f'{name}: no SHA-256 digest in GitHub Releases - the installer refuses it')
        continue
    digest = digest.split(':', 1)[1].lower()
    path = os.path.join(tmp, name)
    urllib.request.urlretrieve(url, path)
    if sha256(path) != digest:
        fail(f'{name}: downloaded file does not match its digest')
        continue
    local = os.path.join(local_dir, name)
    note = ''
    if os.path.isfile(local):
        if sha256(local) != digest:
            fail(f'{name}: differs from the local build {local}')
            continue
        note = ', same as the local build'
    if name.endswith('.deb'):
        problem = deb_problem(path)
    else:
        problem = portable_problem(path, machines[name.split('-linux-')[1].split('-')[0]])
    if problem:
        fail(f'{name}: {problem}')
    else:
        ok(f'{name}: sha256 {digest[:16]}...{note}')

# Wybor instalatora bez --version: najnowsze stabilne wydanie z paczka dla danej architektury.
with open(os.path.join(tmp, 'releases.json'), encoding='utf-8') as source:
    releases = json.load(source)
for arch in machines:
    candidates = []
    for item in releases:
        item_version = item.get('tag_name', '').removeprefix('v')
        if item.get('draft') or item.get('prerelease') or not re.fullmatch(r'\d+\.\d+\.\d+', item_version):
            continue
        if any(asset.get('name') == f'retractordb-{item_version}-linux-{arch}-portable.tar.gz'
               for asset in item.get('assets', [])):
            candidates.append((tuple(map(int, item_version.split('.'))), item['tag_name']))
    chosen = max(candidates)[1] if candidates else None
    if chosen == tag:
        ok(f'Installer default for {arch}: {tag}')
    else:
        warn(f'Installer default for {arch} is {chosen}, not {tag}')

sys.exit(1 if failures else 0)
PY

if ((failures > 0)); then
  echo "Release $tag is NOT ready for users." >&2
  exit 1
fi
echo "Release $tag is complete and ready for users."
