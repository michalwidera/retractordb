#!/usr/bin/env bash
# Sprawdza i instaluje narzedzia potrzebne do render.sh:
# elan, toolchain Lean z pliku lean-toolchain, zaleznosci Lake (Mathlib, Verso),
# python3 i pandoc. Kazdy krok jest idempotentny - drugie uruchomienie niczego nie zmienia.
#
#   ./install-lean.sh                    instalacja z pobraniem gotowych plikow .olean Mathlib;
#                                        sprawdza tez nowsze wersje elana oraz Lean/Mathlib/Verso
#                                        i w terminalu pyta, czy je zainstalowac
#   ./install-lean.sh --no-cache         bez `lake exe cache get` (Mathlib kompilowany lokalnie)
#   ./install-lean.sh --upgrade          podnosi bez pytania elana oraz Lean, Mathlib i Verso do
#                                        najnowszej stabilnej wersji wspolnej dla wszystkich trzech;
#                                        po bledzie lub ostrzezeniu (np. deprecated) w
#                                        `lake build --wfail` przywraca poprzednie wersje
#   ./install-lean.sh --no-upgrade-check nie sprawdza nowszych wersji (bez sieci, szybciej)
set -euo pipefail

cd "$(dirname "$0")"
./link-lake.sh

use_cache=1
upgrade=0
check=1
for arg in "$@"; do
  case "$arg" in
    --no-cache) use_cache=0 ;;
    --upgrade) upgrade=1 ;;
    --no-upgrade-check) check=0 ;;
    *)
      echo "uzycie: $0 [--no-cache] [--upgrade | --no-upgrade-check]" >&2
      exit 2
      ;;
  esac
done
((upgrade && !check)) && {
  echo "--upgrade i --no-upgrade-check wykluczaja sie" >&2
  exit 2
}

info() { printf '==> %s\n' "$*"; }
die() {
  printf 'BLAD: %s\n' "$*" >&2
  exit 1
}

# Tagi wydan stabilnych vX.Y.Z (bez -rc) repozytorium, posortowane leksykalnie dla `comm`.
stable_tags() {
  git ls-remote --tags --refs "$1" | sed -n 's#.*refs/tags/\(v[0-9]*\.[0-9]*\.[0-9]*\)$#\1#p' | sort -u
}

# Czy wersja $1 jest nowsza od $2.
newer() {
  [[ "$1" != "$2" && "$(printf '%s\n%s\n' "$1" "$2" | sort -V | tail -1)" == "$1" ]]
}

# Zgoda na aktualizacje: --upgrade = tak; terminal = pytanie; bez terminala = tylko informacja.
want_upgrade() {
  local reply
  ((upgrade)) && return 0
  if [[ -t 0 ]]; then
    read -r -p "    $1 Zainstalowac? [t/N] " reply
    [[ "$reply" == [tTyY]* ]]
  else
    info "$1 Uruchom z --upgrade, aby zainstalowac."
    return 1
  fi
}

fetch_deps() {
  if ((use_cache)); then
    lake exe cache get
  else
    # Wczytanie workspace klonuje pakiety wedlug manifestu, bez jego zmiany.
    lake env true
  fi
}

# 1. Pakiety systemowe: narzedzia i pliki LaTeX-a, ktorych wymaga PDF z Verso (lualatex + memoir)
declare -A apt_for=(
  [curl]=curl [git]=git [python3]=python3 [pandoc]=pandoc
  [lualatex]=texlive-luatex [latexmk]=latexmk
  [memoir.cls]=texlive-latex-recommended [fancyvrb.sty]=texlive-latex-recommended
  [tcolorbox.sty]=texlive-latex-extra [fvextra.sty]=texlive-latex-extra [newunicodechar.sty]=texlive-latex-extra
  [sourceserifpro.sty]=texlive-fonts-extra [sourcesanspro.sty]=texlive-fonts-extra
  [sourcecodepro.sty]=texlive-fonts-extra [ulem.sty]=texlive-plain-generic
  [DejaVuSansMono.ttf]=fonts-dejavu-core
)
missing=()
for item in "${!apt_for[@]}"; do
  if [[ "$item" == *.ttf ]]; then
    # Bez potoku do `grep -q`: przy pipefail SIGPIPE w fc-list dalby falszywy brak.
    [[ "$(fc-list : file 2>/dev/null)" == *"/${item}:"* ]] && continue
  elif [[ "$item" == *.* ]]; then
    command -v kpsewhich >/dev/null 2>&1 && [[ -n "$(kpsewhich "$item")" ]] && continue
  else
    command -v "$item" >/dev/null 2>&1 && continue
  fi
  missing+=("${apt_for[$item]}")
done
if ((${#missing[@]})); then
  mapfile -t missing < <(printf '%s\n' "${missing[@]}" | sort -u)
  command -v apt-get >/dev/null 2>&1 || die "brak pakietow: ${missing[*]}; zainstaluj recznie"
  info "instaluje pakiety systemowe: ${missing[*]}"
  sudo apt-get update
  sudo apt-get install -y "${missing[@]}"
else
  info "pakiety systemowe: OK (curl git python3 pandoc lualatex latexmk + pakiety LaTeX-a)"
fi

# 2. elan
export PATH="${ELAN_HOME:-$HOME/.elan}/bin:$PATH"
if command -v elan >/dev/null 2>&1; then
  info "elan: OK ($(elan --version))"
  if ((check)); then
    elan_have="$(elan --version | awk '{print $2}')"
    if ! elan_best="$(stable_tags https://github.com/leanprover/elan.git | sort -V | tail -1)"; then
      info "nie udalo sie sprawdzic wersji elana - pomijam"
    elif newer "${elan_best#v}" "$elan_have" && want_upgrade "dostepny elan ${elan_best#v} (jest $elan_have)."; then
      elan self update || info "elan self update nieudane - elan zostaje w wersji $elan_have"
    fi
  fi
else
  info "instaluje elan"
  curl -sSfL https://raw.githubusercontent.com/leanprover/elan/master/elan-init.sh |
    sh -s -- -y --default-toolchain none
  command -v elan >/dev/null 2>&1 || die "elan nie jest dostepny po instalacji"
  info "elan: zainstalowany ($(elan --version)); dodaj ~/.elan/bin do PATH w nowej powloce"
fi

# 3. Toolchain Lean przypiety w lean-toolchain
[[ -f lean-toolchain ]] || die "brak pliku lean-toolchain w $(pwd)"
toolchain="$(tr -d '[:space:]' <lean-toolchain)"
info "toolchain: $toolchain"
elan toolchain install "$toolchain"

current="${toolchain##*:v}"
have="$(lean --version)"
[[ "$have" == "Lean (version $current,"* ]] || die "lean w tym katalogu to '$have', oczekiwano $current"
info "lean: OK ($have)"
lake --version >/dev/null || die "lake nie dziala"

# 4. Sprawdzenie nowszej wersji Lean/Mathlib/Verso
# Najnowsza stabilna wersja wydana przez lean4 i wszystkie przypiete zaleznosci; pusto przy bledzie.
latest_common() {
  local common url
  common="$(stable_tags https://github.com/leanprover/lean4.git)" || return 1
  for url in "${require_urls[@]}"; do
    common="$(comm -12 <(printf '%s\n' "$common") <(stable_tags "$url"))" || return 1
  done
  printf '%s\n' "$common" | sed '/^$/d' | sort -V | tail -1
}

target=""
if ((check)); then
  mapfile -t require_urls < <(sed -n -E 's/^require .* from git "([^"]+)" @ "v[^"]+".*/\1/p' lakefile.lean)
  mapfile -t require_tags < <(sed -n -E 's/^require .* from git "[^"]+" @ "(v[^"]+)".*/\1/p' lakefile.lean)
  pinned=1
  for tag in "${require_tags[@]}"; do
    [[ "$tag" == "v$current" ]] || pinned=0
  done

  info "sprawdzam najnowsza stabilna wersje wspolna dla lean4 i ${#require_urls[@]} zaleznosci"
  if ((!pinned)); then
    msg="lakefile.lean przypina ${require_tags[*]}, a lean-toolchain v$current; ujednolic recznie"
    ((upgrade)) && die "$msg"
    info "$msg - pomijam sprawdzenie"
  elif ! best="$(latest_common)" || [[ -z "$best" ]]; then
    ((upgrade)) && die "nie udalo sie ustalic wspolnej wersji stabilnej"
    info "nie udalo sie ustalic wspolnej wersji stabilnej - pomijam"
  elif ! newer "${best#v}" "$current"; then
    info "brak aktualizacji: v$current jest najnowsza wspolna wersja stabilna"
  elif want_upgrade "dostepna wersja ${best} (jest v$current) dla Lean, Mathlib i Verso."; then
    target="${best#v}"
  fi
fi

if [[ -z "$target" ]]; then
  # 5. Zaleznosci Lake zgodne z lake-manifest.json
  info "pobieram zaleznosci Lake"
  fetch_deps
  info "gotowe - teraz ./render.sh"
  exit 0
fi

info "podnosze v$current -> v$target"
backup="$(mktemp -d)"
cp lean-toolchain lakefile.lean lake-manifest.json "$backup/"
sed -i -E "s/^(require .* from git \"[^\"]+\" @ )\"v$current\"/\1\"v$target\"/" lakefile.lean

# `lake update` sam przepisuje lean-toolchain na toolchain zaleznosci i restartuje sie przez elan.
if lake update &&
  [[ "$(tr -d '[:space:]' <lean-toolchain)" == "leanprover/lean4:v$target" ]] &&
  fetch_deps &&
  lake build Profs profs_manual --wfail; then
  rm -rf "$backup"
  info "gotowe: v$target; zmienione lean-toolchain, lakefile.lean, lake-manifest.json - przejrzyj i zacommituj"
  exit 0
fi

info "aktualizacja do v$target nieudana - przywracam v$current"
cp "$backup"/lean-toolchain "$backup"/lakefile.lean "$backup"/lake-manifest.json .
rm -rf "$backup"
fetch_deps
die "pozostaje v$current; przyczyna w logu powyzej"
