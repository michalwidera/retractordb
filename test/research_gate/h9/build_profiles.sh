#!/usr/bin/env bash
# Buduje profile ablacyjne K26v3 z profiles.tsv w izolowanych katalogach
# build/K26v3-<slug> repozytorium kodu i weryfikuje --build-info bajtowo.
#
# KOPIA aparatury K6c z DWIEMA zmianami (i tylko nimi):
#   1. liczba profili nie jest już zaszyta jako 5 — wynika z profiles.tsv, bo
#      K26v3 ma cztery profile (`OFF` nie jest kontrolą przyczynową H9);
#      reguła "zero zbudowanych profili nie jest sukcesem" zachowana jawnie;
#   2. prefiks katalogu builda K6- → K26v3-, żeby kampania nie mieszała binariów
#      z K6c ani K23 w tym samym drzewie kodu.
# Nazwy zmiennych środowiskowych zostają K6_*, bo zmiana ich nazw nie ma pokrycia
# w żadnym uruchomieniu — harness jest przeniesiony, nie przepisany.
#
# Ten sam skrypt działa na nadzorcy (kontrola wejściowa liczników, compile-only)
# i na workerze (kampania pomiarowa). Różnicę robią zmienne środowiskowe:
#
#   RDB_CODE_REPO   repozytorium kodu (domyślnie ../../retractordb)
#   K6_BUILD_JOBS   równoległość builda (domyślnie 4)
#   K6_SETCAP=1     nadaje cap_sys_nice+cap_ipc_lock każdej binarce (worker, R7)
#   K6_RUN_CTEST=1  uruchamia macierz it_optimizer_ablation w każdym profilu
#   K6_CCACHE=1     kompilator przez ccache
#   K6_CPUS         lista rdzeni dla builda (np. 0-3); domyślnie bez przypięcia
#   K6_RAW_DIR      katalog na logi (domyślnie results/raw/profiles)
#
# `K6_CCACHE` jest bezpieczne dla kampanii pomiarowej: przełączniki `RDB_OPT_*`
# są definicjami **wyłącznie celu `retractor`** (`src/retractor/lib/CMakeLists.txt`),
# więc reszta drzewa ma między profilami identyczne wyjście preprocesora i trafia
# w cache. ccache nie zmienia wynikowej binarki — `--build-info` i tak jest
# weryfikowane bajtowo dla każdego profilu.
#
# `K6_RUN_CTEST` wymaga zbudowania WSZYSTKICH celów, co na Raspberry Pi trwa
# godziny. Macierz funkcjonalną można wykonać na nadzorcy dla tego samego
# commita (artefakty są bajtowo identyczne między architekturami, K18), a worker
# buduje wtedy tylko binarki pomiarowe.
#
# Zgodnie z R2 skrypt nie zapisuje niczego do repozytorium kodu poza katalogami
# build/K6-*, które są wyłączone z odcisku drzewa (lib/common.sh).
set -euo pipefail

cd "$(dirname "$0")"
here=$(pwd)
# Bramka zyje w drzewie silnika (test/research_gate/h9), nie w repozytorium
# eksperymentu: repozytorium kodu to trzy poziomy wyzej, a logi builda ida do
# build/, ktore jest w .gitignore. Wyjscia uruchomienia nie wchodza do zrodel.
code_repo=${RDB_CODE_REPO:-"$here/../../.."}
code_repo=$(realpath "$code_repo")
jobs=${K6_BUILD_JOBS:-4}
raw_dir=${K6_RAW_DIR:-"$code_repo/build/gate-profiles-logs"}

[ -f "$code_repo/CMakeLists.txt" ] && [ -d "$code_repo/src" ] \
  || { echo "BLAD: $code_repo nie wyglada na drzewo zrodel RetractorDB" >&2; exit 1; }

# Przypięcie builda. W czasie budowania nie trwa żaden pomiar, więc wolno użyć
# także izolowanego rdzenia 3 — ale nigdy w trakcie kampanii.
build_wrapper=()
if [ -n "${K6_CPUS:-}" ]; then
  build_wrapper=(taskset -c "$K6_CPUS")
fi
ccache_args=()
if [ "${K6_CCACHE:-0}" = "1" ]; then
  command -v ccache >/dev/null || { echo "BLAD: K6_CCACHE=1, ale brak ccache" >&2; exit 1; }
  ccache_args=(-DCMAKE_CXX_COMPILER_LAUNCHER=ccache -DCMAKE_C_COMPILER_LAUNCHER=ccache)
fi

# Z 600-liniowego lib/common.sh aparatury eksperymentu ten skrypt uzywa trzech
# funkcji. Sa tu PRZENIESIONE DOSLOWNIE, nie przepisane: verify_probe_binary_profile
# porownuje `--build-info` bajtowo i to porownanie jest jedynym dowodem, ze
# zbudowany profil jest tym, za ktory sie podaje.
log()  { printf '[%(%Y-%m-%d %H:%M:%S)T] %s\n' -1 "$*" >&2; }
die()  { log "BLAD: $*"; exit 1; }

verify_probe_binary_profile() {
  local binary="$1" dedup="$2" share="$3" commutative="$4" factor="$5" simplify="${6:-}"
  local actual expected
  [ -x "$binary" ] || {
    log "BLAD: brak wykonywalnej binarki xretractor: $binary"
    return 1
  }
  actual=$("$binary" --build-info 2>/dev/null) || {
    log "BLAD: $binary --build-info nie powiodlo sie"
    return 1
  }
  expected=$(printf '%s\n' \
    "RDB_OPT_DEDUP_SUBSTRATES=$dedup" \
    "RDB_OPT_SHARE_EQUIVALENT_SELECTS=$share" \
    "RDB_OPT_COMMUTATIVE_ADD=$commutative" \
    "RDB_OPT_FACTOR_MATCHED_HASH_TIMEMOVES=$factor" \
    "RDB_BENCH_PROBE=ON")
  if [ -n "$simplify" ]; then
    expected=$(printf '%s\n%s\n' "$expected" "RDB_OPT_SIMPLIFY_EXPRESSIONS=$simplify")
  fi
  [ "$actual" = "$expected" ] || {
    log "BLAD: $binary nie jest oczekiwanym profilem ($dedup/$share/$commutative/$factor)"
    diff -u <(printf '%s\n' "$expected") <(printf '%s\n' "$actual") >&2 || true
    return 1
  }
}

conan_dir="$code_repo/build/Conan-K6-Profiles"
toolchain="$conan_dir/build/Release/generators/conan_toolchain.cmake"
mkdir -p "$raw_dir"

if [ ! -f "$toolchain" ]; then
  log "conan install (jednorazowo dla wszystkich profili)"
  conan install "$code_repo" \
    -s build_type=Release \
    --build missing \
    -of "$conan_dir" >"$raw_dir/conan-install.log" 2>&1
fi

built=0
while IFS=$'\t' read -r profile slug dedup share commutative factor; do
  [ "$profile" = "profile" ] && continue
  [ -n "$profile" ] || continue

  build_dir="$code_repo/build/K26v3-$slug"
  log "profil $profile -> $build_dir"
  cmake -S "$code_repo" -B "$build_dir" -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_TOOLCHAIN_FILE="$toolchain" \
    "${ccache_args[@]}" \
    -DRDB_OPT_DEDUP_SUBSTRATES="$dedup" \
    -DRDB_OPT_SHARE_EQUIVALENT_SELECTS="$share" \
    -DRDB_OPT_COMMUTATIVE_ADD="$commutative" \
    -DRDB_OPT_FACTOR_MATCHED_HASH_TIMEMOVES="$factor" \
    -DRDB_OPT_SIMPLIFY_EXPRESSIONS=ON \
    -DRDB_BENCH_PROBE=ON >"$raw_dir/cmake-$slug.log" 2>&1

  "${build_wrapper[@]}" cmake --build "$build_dir" --target xretractor \
    --parallel "$jobs" >"$raw_dir/build-$slug.log" 2>&1

  binary="$build_dir/src/retractor/xretractor"
  verify_probe_binary_profile "$binary" "$dedup" "$share" "$commutative" "$factor" ON ||
    die "profil $profile nie potwierdza się w --build-info"
  "$binary" --build-info >"$raw_dir/build-info-$slug.txt"

  if [ "${K6_SETCAP:-0}" = "1" ]; then
    # R7: sonda mierzy pod SCHED_FIFO i mlockall, więc capabilities muszą być
    # na KAŻDEJ z czterech binarek, nie tylko na zainstalowanej.
    sudo -n setcap cap_sys_nice,cap_ipc_lock+ep "$binary" ||
      die "nie można nadać capabilities RT na $binary"
    getcap "$binary" | grep -q "cap_ipc_lock,cap_sys_nice=ep\|cap_sys_nice,cap_ipc_lock=ep" ||
      die "binarka $binary nie ma wymaganych capabilities RT"
  fi

  if [ "${K6_RUN_CTEST:-0}" = "1" ]; then
    "${build_wrapper[@]}" cmake --build "$build_dir" --parallel "$jobs" >>"$raw_dir/build-$slug.log" 2>&1
    ctest --test-dir "$build_dir" \
      -R '^it_optimizer_ablation-' \
      --output-on-failure >"$raw_dir/ctest-$slug.log" 2>&1 ||
      die "macierz it_optimizer_ablation nie przechodzi w profilu $profile"
  fi

  built=$((built + 1))
done < profiles.tsv

# Reguła zliczania: zero zbudowanych profili nie jest sukcesem. Oczekiwana liczba
# pochodzi z profiles.tsv, więc dopisanie profilu nie wymaga zmiany skryptu, ale
# pusty albo obcięty profiles.tsv nadal jest błędem, a nie cichym sukcesem.
expected=$(awk 'NR > 1 && NF { n++ } END { print n + 0 }' profiles.tsv)
[ "$expected" -ge 1 ] || die "profiles.tsv nie zawiera żadnego profilu"
[ "$built" -eq "$expected" ] || die "zbudowano $built profili, oczekiwano $expected"
log "profile zbudowane i zweryfikowane: $built"
