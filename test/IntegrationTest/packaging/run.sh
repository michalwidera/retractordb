#!/bin/bash
# Test poprawności pakietów CPack: pakiet binarny DEB zawiera DOKŁADNIE wymagany
# zestaw plików (3 binaria + jednostka systemd + przykłady konfiguracji) —
# żadnych nadmiarowych.
# Sprawdzamy tylko DEB (a nie TGZ) — whitelist jest identyczny dla obu (te same
# reguły install + prefiks /usr), a budowanie obu podwajało czas (kompresja
# dużych binariów Debug ~5 s/pakiet).
#
# Pakietu ŹRÓDŁOWEGO nie ma — został wyłączony, bo jego zawartość była
# nieprawdziwa (brakowało w nim rdzenia silnika), a ta kontrola tego nie
# wykrywała: sprawdzała wyłącznie nieobecność artefaktów, nigdy obecność
# źródeł. Powód i warunki powrotu opisuje komentarz przy CPACK_SOURCE_* w
# głównym CMakeLists.txt.
# $1 = izolowany katalog wyjściowy, $2 = CMAKE_BINARY_DIR (z konfiguracjami CPack).
set -e

OUT="$1"
CFG="$2/CPackConfig.cmake"
rm -rf "$OUT"
mkdir -p "$OUT"
test -f "$CFG"

# Dokładny, wymagany zestaw plików w pakiecie binarnym (bez katalogów).
EXPECTED=$(printf '%s\n' \
  usr/bin/xqry \
  usr/bin/xretractor \
  usr/bin/xtrdb \
  usr/lib/systemd/system/xretractor.service \
  usr/share/retractordb/retractor.toml.example \
  usr/share/retractordb/service.env.example | sort)

assert_exact() { # $1 = opis pakietu, $2 = lista plików (po jednym na linię)
  got=$(printf '%s\n' "$2" | sort)
  if [ "$got" != "$EXPECTED" ]; then
    echo "BŁĄD: $1 ma inny zestaw plików niż wymagany (< wymagane, > faktyczne):"
    diff <(printf '%s\n' "$EXPECTED") <(printf '%s\n' "$got") || true
    exit 1
  fi
}

# --- Pakiet binarny: DEB (wymaga dpkg-deb do inspekcji zawartości) ---
if command -v dpkg-deb >/dev/null 2>&1; then
  cpack -G DEB -B "$OUT" --config "$CFG"
  DEB=$(ls "$OUT"/*.deb | head -1)
  test -n "$DEB"
  assert_exact "DEB" "$(dpkg-deb -c "$DEB" | awk '$1 !~ /^d/ {print $NF}' | grep -oE 'usr/.*')"
else
  echo "POMINIĘTO weryfikację pakietu binarnego: brak dpkg-deb"
fi

echo "packaging artifact check OK"
