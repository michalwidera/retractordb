#!/bin/bash
# Jeden przebieg budowy i testow na macOS, z pelnym dziennikiem w pliku.
#
# Po co osobny skrypt, skoro jest scripts/buildrdb.sh: ten sluzy do BRING-UPU.
# Nie zatrzymuje sie na pierwszym bledzie, tylko przechodzi przez wszystkie etapy
# i zapisuje calosc do jednego pliku, zeby z jednego uruchomienia dalo sie
# odczytac komplet usterek zamiast odkrywac je po jednej na przebieg. Przed
# etapami wypisuje przeglad srodowiska - wersje narzedzi i te dwie wartosci
# z naglowkow systemowych, ktore decyduja o ksztalcie portu (patrz PRZEGLAD).
#
# Uzycie:
#   scripts/macos-build.sh                 # Debug, pelny przebieg
#   scripts/macos-build.sh release         # to samo w konfiguracji Release
#   scripts/macos-build.sh --skip-tests    # sama budowa
#   scripts/macos-build.sh --sanitize      # z -DRDB_SANITIZE=address,undefined
#   scripts/macos-build.sh --no-install    # nie doinstalowuj brakujacych narzedzi
#
# Dziennik: build/macos-build.log (sciezka wypisywana na koncu).

set -o pipefail

script_dir=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
source_dir=$(cd "$script_dir/.." && pwd)

build_type=Debug
run_tests=1
sanitize=""
auto_install=1

# Podloga wersji CMake. NIE jest ozdobna: przy zachowaniu polityki sprzed 4.4
# (CMP0219) ginie backslash w argumencie `add_test` przeslonietego makrem, przez
# co `grep \'\\.shadow$\'` staje sie `grep \'.shadow$\'` i testy integracyjne
# przechodza z zupelnie innym wzorcem. Prog stoi w trzech miejscach naraz -
# tutaj, w conanfile.py i w RDB_CMAKE_MIN_VERSION w scripts/buildrdb.sh.
cmake_floor="4.4.2"

for arg in "$@"; do
  case "$arg" in
    release | Release) build_type=Release ;;
    debug | Debug) build_type=Debug ;;
    --skip-tests) run_tests=0 ;;
    --sanitize) sanitize="address,undefined" ;;
    --no-install) auto_install=0 ;;
    -h | --help)
      sed -n '2,20p' "${BASH_SOURCE[0]}"
      exit 0
      ;;
    *)
      echo "macos-build: nieznany argument '$arg'" >&2
      exit 2
      ;;
  esac
done

build_dir="$source_dir/build/$build_type"
log_dir="$source_dir/build"
log_file="$log_dir/macos-build.log"
mkdir -p "$log_dir"
: >"$log_file"

failures=0

banner() {
  {
    echo
    echo "=================================================================="
    echo "== $*"
    echo "=================================================================="
  } | tee -a "$log_file"
}

# Etap: wolany z opisem i poleceniem. Nie przerywa przebiegu - liczy usterki,
# zeby jedno uruchomienie pokazalo wszystkie, a nie tylko pierwsza.
stage() {
  local name="$1"
  shift
  banner "$name"
  echo "+ $*" | tee -a "$log_file"
  if "$@" 2>&1 | tee -a "$log_file"; then
    echo "-- OK: $name" | tee -a "$log_file"
    return 0
  fi
  failures=$((failures + 1))
  echo "-- BLAD: $name" | tee -a "$log_file"
  return 1
}

note() { echo "$*" | tee -a "$log_file"; }

if [ "$(uname -s)" != "Darwin" ]; then
  echo "macos-build.sh dziala wylacznie na macOS (uname -s = $(uname -s))." >&2
  exit 2
fi

banner "PRZEGLAD SRODOWISKA"
{
  echo "data:            $(date '+%Y-%m-%d %H:%M:%S %z')"
  echo "system:          $(sw_vers -productName) $(sw_vers -productVersion) ($(sw_vers -buildVersion))"
  echo "architektura:    $(uname -m)"
  echo "rdzenie:         $(sysctl -n hw.ncpu)"
  echo "pamiec:          $(( $(sysctl -n hw.memsize) / 1024 / 1024 )) MiB"
  echo "drzewo zrodel:   $source_dir"
  echo "katalog budowy:  $build_dir"
  echo "konfiguracja:    $build_type"
  echo
  echo "-- narzedzia --"
  for tool in xcodebuild xcrun clang clang++ cc c++ cmake ninja conan python3 brew git ccache; do
    if command -v "$tool" >/dev/null 2>&1; then
      printf '%-12s %s\n' "$tool" "$(command -v "$tool")"
    else
      printf '%-12s %s\n' "$tool" "BRAK"
    fi
  done
  echo
  echo "-- wersje --"
  # Skad bierze sie kompilator: pelny Xcode.app czy same Command Line Tools.
  # Do tej budowy wystarczaja CLT (clang + SDK to te same pliki), ale `xcodebuild`
  # dziala wylacznie przy wybranym Xcode.app - i to jest cala roznica, ktora tu widac.
  echo "xcode-select -p: $(xcode-select -p 2>/dev/null)"
  if xcodebuild -version >/dev/null 2>&1; then
    xcodebuild -version
  elif [ -d /Applications/Xcode.app ]; then
    echo "xcodebuild: niedostepny, choc /Applications/Xcode.app istnieje"
    echo "            (przelaczenie: sudo xcode-select -s /Applications/Xcode.app)"
  else
    echo "xcodebuild: brak - w systemie sa same Command Line Tools"
    echo "            (do tej budowy wystarcza; xcodebuild nie jest tu do niczego potrzebny)"
  fi
  echo "SDK: $(xcrun --show-sdk-version 2>/dev/null) w $(xcrun --show-sdk-path 2>/dev/null)"
  c++ --version 2>/dev/null | head -2
  cmake --version 2>/dev/null | head -1
  ninja --version 2>/dev/null | sed 's/^/ninja /'
  conan --version 2>/dev/null
  echo "MACOSX_DEPLOYMENT_TARGET=${MACOSX_DEPLOYMENT_TARGET:-<nieustawiony>}"
} 2>&1 | tee -a "$log_file"

banner "PRZEGLAD NAGLOWKOW SYSTEMOWYCH"
# Te dwie wartosci rozstrzygaja o ksztalcie portu, a nie daja sie zgadnac z
# dokumentacji: _POSIX_SHARED_MEMORY_OBJECTS decyduje, czy Boost.Interprocess
# uzywa shm_open czy zwyklych plikow (a wiec gdzie sa obiekty IPC i czy kontrole
# higieny w testach maja czego szukac), a PSHMNAMLEN podaje limit dlugosci nazwy
# obiektu shm_open, ktory na macOS jest o rzad wielkosci mniejszy niz na Linuksie.
probe_dir=$(mktemp -d "${TMPDIR:-/tmp}/rdbprobe.XXXXXXXX")
cat >"$probe_dir/probe.cpp" <<'PROBE'
#include <unistd.h>
#include <sys/posix_shm.h>
#include <cstdio>
int main() {
#ifdef _POSIX_SHARED_MEMORY_OBJECTS
  std::printf("_POSIX_SHARED_MEMORY_OBJECTS = %ld\n", (long)_POSIX_SHARED_MEMORY_OBJECTS);
#else
  std::printf("_POSIX_SHARED_MEMORY_OBJECTS = <niezdefiniowane>\n");
#endif
#ifdef PSHMNAMLEN
  std::printf("PSHMNAMLEN                   = %d\n", (int)PSHMNAMLEN);
#else
  std::printf("PSHMNAMLEN                   = <niezdefiniowane>\n");
#endif
  std::printf("_SC_NPROCESSORS_ONLN         = %ld\n", sysconf(_SC_NPROCESSORS_ONLN));
  return 0;
}
PROBE
if c++ -std=c++23 -o "$probe_dir/probe" "$probe_dir/probe.cpp" 2>>"$log_file"; then
  "$probe_dir/probe" 2>&1 | tee -a "$log_file"
else
  note "sonda naglowkow nie skompilowala sie - patrz dziennik wyzej"
  failures=$((failures + 1))
fi
rm -rf "$probe_dir"

# --- brakujace narzedzia -------------------------------------------------
banner "NARZEDZIA BUDOWY"
# Conan, Ninja i CMake sa wymagane; ccache tylko przyspiesza. Instalujemy przez
# Homebrew, bo python z Homebrew odmawia `pip install` poza srodowiskiem wirtualnym
# (PEP 668), wiec sciezka pipowa i tak konczylaby sie tutaj.
#
# CMake jest na tej liscie, chociaz conanfile.py przypina wlasny (tool_requires
# cmake/[>=4.4.2]) i to ON liczy sie przy budowie: `conanbuild.sh` nizej stawia go
# na poczatku PATH. Ale `conan install --build missing` na ZIMNYM cache buduje
# recepty zaleznosci, a receptura, ktora nie deklaruje wlasnego tool_requires,
# siega po cmake z PATH - wiec jakis cmake musi tu byc WCZESNIEJ. Na maszynie
# developera zwykle juz jest i dlatego ta sciezka nie byla przebiegnieta do
# 2026-09-20, kiedy oblal pierwszy job macOS na CircleCI: lista instalacyjna miala
# `conan ninja`, a lista kontrolna ponizej `conan ninja cmake`. Obie MUSZA byc te
# same - inaczej skrypt sprawdza cos, czego nie zainstalowal.
missing=""
for tool in conan ninja cmake; do
  command -v "$tool" >/dev/null 2>&1 || missing="$missing $tool"
done
if [ -n "$missing" ]; then
  note "brakuje:$missing"
  if [ "$auto_install" = "0" ]; then
    note "--no-install: przerywam. Zainstaluj recznie: brew install$missing"
    note "Dziennik: $log_file"
    exit 1
  fi
  if ! command -v brew >/dev/null 2>&1; then
    note "brak Homebrew - zainstaluj recznie:$missing"
    note "Dziennik: $log_file"
    exit 1
  fi
  banner "brew install$missing"
  # Kodu wyjscia `brew install` NIE traktujemy jako rozstrzygniecia. Homebrew konczy
  # sie niezerowo takze wtedy, gdy instalacja sie UDALA, a on ma jeszcze cos do
  # powiedzenia - podpowiedz o przeslonietych plikach wykonywalnych albo nieudany
  # `brew link` zaleznosci. Rozstrzyga jedyne pytanie, ktore nas dotyczy: czy
  # narzedzie jest teraz na PATH.
  #
  # Zmienne HOMEBREW_*: cisza w dzienniku (samo auto-update potrafi zajac w nim
  # 300 linii) i brak kontroli przesloniec, ktora jest zrodlem tego falszywego bledu.
  # shellcheck disable=SC2086
  HOMEBREW_NO_AUTO_UPDATE=1 HOMEBREW_NO_ENV_HINTS=1 HOMEBREW_NO_PATH_SHADOW_CHECK=1 \
    brew install $missing 2>&1 | tail -40 | tee -a "$log_file"
  hash -r
fi
still_missing=""
for tool in conan ninja cmake; do
  command -v "$tool" >/dev/null 2>&1 || still_missing="$still_missing $tool"
done
if [ -n "$still_missing" ]; then
  note "nadal brak:$still_missing - zainstaluj recznie i uruchom ponownie"
  note "Dziennik: $log_file"
  exit 1
fi
for tool in conan ninja cmake; do
  note "$(printf '%-8s %s' "$tool" "$(command -v "$tool")")"
done

# --- profil Conana -------------------------------------------------------
banner "PROFIL CONANA"
conan profile detect --exist-ok 2>&1 | tee -a "$log_file"
profile="$HOME/.conan2/profiles/default"

# Conan zna skonczona liste wersji kompilatorow i odrzuca nieznana bledem
# "Invalid setting". Apple wydaje nowego clanga szybciej, niz Conan dopisuje go do
# settings.yml, wiec swiezy Xcode potrafi zablokowac budowe na czyms, co nie ma nic
# wspolnego z tym projektem. Uzupelniamy brak przez settings_user.yml - to jest
# MECHANIZM DO TEGO PRZEZNACZONY, wiec nie ruszamy settings.yml Conana i nic nie
# psuje sie przy jego aktualizacji.
python3 - "$profile" <<'PY' 2>&1 | tee -a "$log_file"
import os, re, sys

profile = sys.argv[1]
home = os.path.expanduser("~/.conan2")
settings = os.path.join(home, "settings.yml")
user_settings = os.path.join(home, "settings_user.yml")

if not os.path.exists(profile) or not os.path.exists(settings):
    print("conan: brak profilu albo settings.yml - pomijam sprawdzenie wersji kompilatora")
    raise SystemExit(0)

text = open(profile, encoding="utf-8").read()
compiler = re.search(r"^compiler=(.+)$", text, re.M)
version = re.search(r"^compiler\.version=(.+)$", text, re.M)
if not compiler or not version:
    raise SystemExit(0)
compiler, version = compiler.group(1).strip(), version.group(1).strip()

known = open(settings, encoding="utf-8").read()
block = re.search(r"^    %s:\n(?:.*\n)*?(?=^    \S|^\S)" % re.escape(compiler), known, re.M)
if block and re.search(r'[\[,]\s*"?%s"?\s*[,\]]' % re.escape(version), block.group(0)):
    print("conan: %s %s jest znany" % (compiler, version))
    raise SystemExit(0)

existing = open(user_settings, encoding="utf-8").read() if os.path.exists(user_settings) else ""
if version in existing and compiler in existing:
    print("conan: %s %s juz dopisany w settings_user.yml" % (compiler, version))
    raise SystemExit(0)

with open(user_settings, "a", encoding="utf-8") as handle:
    if existing and not existing.endswith("\n"):
        handle.write("\n")
    handle.write("compiler:\n  %s:\n    version: [\"%s\"]\n" % (compiler, version))
print("conan: %s %s nieznany Conanowi - dopisany do %s" % (compiler, version, user_settings))
PY
if [ -f "$profile" ]; then
  # C++23 w profilu - inaczej Conan buduje zaleznosci pod inny standard niz
  # projekt, co konczy sie rozjazdem ABI dopiero na linkowaniu.
  if ! grep -q 'compiler.cppstd=gnu23' "$profile"; then
    sed 's/compiler\.cppstd=.*/compiler.cppstd=gnu23/' <"$profile" >"$profile.tmp" && mv "$profile.tmp" "$profile"
  fi
  if ! grep -q '^\[conf\]' "$profile"; then printf '\n[conf]\n' >>"$profile"; fi
  if ! grep -q 'tools.cmake.cmaketoolchain:generator' "$profile"; then
    echo 'tools.cmake.cmaketoolchain:generator=Ninja' >>"$profile"
  fi
  sed "s/build_type=.*/build_type=$build_type/" <"$profile" >"$profile.tmp" && mv "$profile.tmp" "$profile"
  cat "$profile" | tee -a "$log_file"
fi

# --- etapy ---------------------------------------------------------------
cmake_args=(-DCMAKE_BUILD_TYPE="$build_type")
[ -n "$sanitize" ] && cmake_args+=(-DRDB_SANITIZE="$sanitize")

stage "conan source" conan source "$source_dir"
stage "conan install ($build_type)" conan install "$source_dir" -s build_type="$build_type" --build missing

toolchain="$build_dir/generators/conan_toolchain.cmake"
if [ ! -f "$toolchain" ]; then
  note "Brak $toolchain - conan install nie doszedl do konca; dalsze etapy pominiete."
  note "Dziennik: $log_file"
  exit 1
fi

# Srodowisko budowy Conana wnosi na PATH narzedzia z tool_requires - przede
# wszystkim PRZYPIETY CMake. Bez tego uzylibysmy tego z PATH-u systemowego, a ten
# bywa starszy niz podloga wersji (patrz cmake_floor wyzej) i wtedy testy
# integracyjne przechodza z innym wzorcem, niz sa napisane.
banner "SRODOWISKO BUDOWY"
if [ -f "$build_dir/generators/conanbuild.sh" ]; then
  # shellcheck disable=SC1091
  . "$build_dir/generators/conanbuild.sh"
  note "zaladowano $build_dir/generators/conanbuild.sh"
fi
hash -r
note "cmake: $(command -v cmake) -> $(cmake --version | head -1)"
note "ninja: $(command -v ninja) -> $(ninja --version 2>/dev/null)"
cmake_have=$(cmake --version | head -1 | sed 's/[^0-9.]*\([0-9.]*\).*/\1/')
lowest=$(printf '%s\n%s\n' "$cmake_floor" "$cmake_have" | sort -V | head -1)
if [ "$lowest" != "$cmake_floor" ]; then
  note "UWAGA: cmake $cmake_have jest ponizej podlogi $cmake_floor."
  note "       Przy politykach sprzed 4.4 testy integracyjne z przeslonietym add_test"
  note "       gubia backslash w argumencie i sprawdzaja inny wzorzec, niz mialy."
  note "       Napraw: brew upgrade cmake   (albo pip install 'cmake>=$cmake_floor')"
  failures=$((failures + 1))
fi

stage "cmake configure" cmake -S "$source_dir" -B "$build_dir" -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE="$toolchain" "${cmake_args[@]}"

jobs=$(sysctl -n hw.ncpu 2>/dev/null || echo 4)
# "-k 0" (ninja: nie przerywaj) jest tu istotne, a nie wygodne. Domyslnie ninja
# staje na pierwszym bledzie, wiec jeden przebieg pokazuje jedna usterke - a przy
# budowie, ktora trwa kilkanascie minut, komplet bledow z jednego przebiegu jest
# roznica miedzy jedna iteracja a dziesiecioma.
stage "cmake build (-j$jobs, keep-going)" cmake --build "$build_dir" --parallel "$jobs" -- -k 0

# Testy integracyjne uruchamiaja binarium ZAINSTALOWANE (patrz test/CLAUDE.md),
# wiec instalacja jest czescia przebiegu, a nie krokiem opcjonalnym.
stage "cmake install" cmake --install "$build_dir"

if [ "$run_tests" = "1" ]; then
  # Rekonfiguracja przed testami: test/CMakeLists.txt kopiuje drzewo testow do
  # katalogu budowy na etapie KONFIGURACJI, a testy integracyjne porownuja
  # artefakty z plikami wzorcowymi z tej kopii.
  stage "cmake reconfigure (odswiezenie kopii testow)" cmake -S "$source_dir" -B "$build_dir"
  stage "cmake build (po rekonfiguracji)" cmake --build "$build_dir" --parallel "$jobs" -- -k 0
  banner "CTEST"
  # --output-on-failure: bez tego z dziennika nie da sie odczytac, CO padlo.
  # --output-junit: ten sam plik, ktory zbiera job linuksowy (patrz run-test w
  # .circleci/config.yml), zeby CircleCI pokazywal wyniki macOS w tej samej
  # zakladce, a nie tylko jako dziennik. Lokalnie to jeden plik wiecej w katalogu
  # budowy i nic poza tym. Powtorka nieudanych nizej junita NIE nadpisuje.
  ( cd "$build_dir" && ctest --output-on-failure -j "$jobs" -LE valgrind --output-junit test_results.xml ) 2>&1 |
    tee -a "$log_file"
  ctest_status=${PIPESTATUS[0]}
  if [ "$ctest_status" -ne 0 ]; then
    failures=$((failures + 1))
    note "-- BLAD: ctest zakonczyl sie kodem $ctest_status"
    # Powtorka SAMYCH nieudanych, pojedynczo i gadatliwie. Przy `-j` ctest
    # przeplata wyjscia testow, a testowi przerwanemu limitem czasu nie pokazuje
    # go wcale - i wtedy z dziennika nie da sie odczytac, CO wlasciwie padlo.
    # Tych testow jest z zalozenia niewiele, wiec powtorka trwa chwile i oszczedza
    # cale kolejne uruchomienie tylko po to, zeby zobaczyc powod.
    banner "POWTORKA NIEUDANYCH TESTOW (pojedynczo, -V)"
    (cd "$build_dir" && ctest --rerun-failed --output-on-failure -V) 2>&1 | tee -a "$log_file"
  else
    note "-- OK: ctest"
  fi
  # `ninja test-api` - JEDYNE wejscie do testow klienta C++ z api/. Cele z tego
  # katalogu sa EXCLUDE_FROM_ALL, a same testy maja etykiete `api`, ktora zwykly
  # przebieg odfiltrowuje: st_api_fake i st_api_real raportuja sie wtedy jako
  # POMINIETE i latwo to przeoczyc w wierszu "100% tests passed". Tymczasem to
  # wlasnie api/cpp/src/client.cpp dostal na Darwinie zamienniki pipe2() oraz
  # environ - jedyny kod pisany pod ten system, ktory bez tego etapu nie zostalby
  # nawet SKOMPILOWANY, nie mowiac o uruchomieniu.
  stage "cmake build test-api (klient C++ + ctest -L api)" \
    cmake --build "$build_dir" --parallel "$jobs" --target test-api

  banner "PODSUMOWANIE CTEST"
  ( cd "$build_dir" && ctest -N 2>&1 | tail -5 ) | tee -a "$log_file"
fi

banner "PODSUMOWANIE"
note "etapy zakonczone bledem: $failures"
note "dziennik: $log_file"
[ "$failures" -eq 0 ] || exit 1
