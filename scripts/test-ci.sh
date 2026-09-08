#!/bin/bash
# shellcheck source-path=SCRIPTDIR
#
# Lokalne odtworzenie jobow CircleCI w kontenerze dockerowym.
#
# Po co: joby `-mydocker` z .circleci/config.yml biegna na obrazie
# micwide/buildenv-retractordb w executorze `large` (4 vCPU / 8 GiB). Maszyna
# deweloperska ma wielokrotnie wiecej rdzeni i inny obraz systemu, wiec awaria
# zalezna od rownoleglosci, pamieci albo wersji pakietow nie odtwarza sie tu z
# definicji. Ten skrypt stawia te sama konfiguracje i wykonuje te same kroki, co
# wybrany job — zeby czerwony CI dalo sie zbadac bez pushowania kolejnych
# commitow "a moze teraz".
#
# ZRODLEM PRAWDY POZOSTAJE .circleci/config.yml. Kazdy profil nizej jest recznym
# odwzorowaniem jednego joba i przy zmianie tamtego pliku trzeba go poprawic —
# nie ma mechanizmu, ktory by to zrobil sam.
#
# Uruchamiac z dowolnego katalogu; drzewo zrodlowe wynika ze sciezki skryptu.

set -o errexit
set -o nounset
set -o pipefail

script_dir=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
repo_root=$(cd "$script_dir/.." && pwd)

if [ ! -f "$repo_root/CMakeLists.txt" ] || [ ! -d "$repo_root/src" ]; then
    echo "Nie jest to drzewo zrodlowe RetractorDB: $repo_root" >&2
    exit 1
fi

# ── Profile ──────────────────────────────────────────────────────────────────
#
# Kolumny: nazwa | job w config.yml | build_type | opis.
# `gate` jest w tej liscie, ale poza domyslnym uzyciem: to najdrozszy job
# przebiegu nocnego (budowa czterech profili ablacji H9 + kampania 2 x 10010
# planow), liczony w dziesiatkach minut takze na goracym ccache.
profile_names=(commit release smoke ablation-all-off ablation-probe-on gate)

profile_job() {
    case "$1" in
        commit)            echo "build-debug-ninja-mydocker (workflow: commit)" ;;
        release)           echo "build-release-ninja-mydocker (L2 manual-nightly-full)" ;;
        smoke)             echo "smoke-debug-ninja-mydocker (L1 manual-nightly-full)" ;;
        ablation-all-off)  echo "build-release-ablation / ablation-all-off (L3)" ;;
        ablation-probe-on) echo "build-release-ablation / ablation-probe-on (L3)" ;;
        gate)              echo "research-gate (L3 manual-nightly-full)" ;;
    esac
}

profile_build_type() {
    case "$1" in
        commit|smoke) echo "Debug" ;;
        *)            echo "Release" ;;
    esac
}

profile_desc() {
    case "$1" in
        commit)            echo "Debug + pelny zestaw testow — to, co idzie po commicie" ;;
        release)           echo "Release + pelny zestaw testow" ;;
        smoke)             echo "sama kompilacja Debug, bez testow" ;;
        ablation-all-off)  echo "Release z piecioma RDB_OPT_* = OFF + pelny zestaw testow" ;;
        ablation-probe-on) echo "Release z RDB_BENCH_PROBE=ON + pelny zestaw testow" ;;
        gate)              echo "bramka badawcza H9/H10 w trybie strict (dlugi przebieg)" ;;
    esac
}

show_list() {
    echo "Profile (odwzorowania jobow z .circleci/config.yml):"
    local p
    for p in "${profile_names[@]}"; do
        printf '  %-18s %-52s %s\n' "$p" "$(profile_job "$p")" "$(profile_desc "$p")"
    done
}

show_help() {
    echo "Uzycie: $0 [--profile <nazwa>] [opcje]"
    echo ""
    show_list
    echo ""
    echo "Opcje:"
    echo "  --profile <nazwa>  Profil do uruchomienia (domyslnie: commit)"
    echo "  --list             Wypisz profile i zakoncz"
    echo "  --out-dir <kat>    Katalog na wyniki testow (domyslnie: build/test-ci)"
    echo "  --image <obraz>    Obraz dockerowy (domyslnie: $default_image)"
    echo "  --cpus <n>         Limit CPU kontenera (domyslnie: $default_cpus — jak resource_class: large)"
    echo "  --memory <rozmiar> Limit RAM kontenera (domyslnie: $default_memory — jak resource_class: large)"
    echo "  --shm-size <r>     Rozmiar /dev/shm (domyslnie: $default_shm_size — domyslna wartosc dockera)"
    echo "  --pull             Wymus pobranie obrazu przed uruchomieniem"
    echo "  --keep             Nie usuwaj kontenera po zakonczeniu (do grzebania recznie)"
    echo "  --reuse-build      Zachowaj katalog build/ miedzy przebiegami (szybko, ale NIE jak CI)"
    echo "  --reset-build      Skasuj zachowany katalog build/ tego profilu i zbuduj od zera"
}

# ── Domyslne parametry ───────────────────────────────────────────────────────
#
# Obraz i limity ida za executorem `debian-mydocker` z `resource_class: large`
# (4 vCPU / 8 GiB). --memory-swap zrownany z --memory: bez tego docker daje
# kontenerowi tyle samo swapu co RAM-u, wiec przekroczenie budzetu objawia sie
# spowolnieniem zamiast zabiciem procesu — a w CI zabija.
default_image="micwide/buildenv-retractordb:latest"
default_cpus="4"
default_memory="8g"
# /dev/shm: 64 MiB to domyslna wartosc dockera i tyle samo ma executor
# dockerowy CircleCI. Wielkosc jest istotna, bo silnik trzyma tam segment
# magistrali i kolejki odpowiedzi (patrz src/retractor/lib/shmBudget.hpp) —
# podniesienie jej tutaj ukryloby odmowe, ktora w CI wystapi.
default_shm_size="64m"

profile="commit"
image="$default_image"
cpus="$default_cpus"
memory="$default_memory"
shm_size="$default_shm_size"
out_dir="$repo_root/build/test-ci"
keep_container=0
force_pull=0
reuse_build=0
reset_build=0

while [ $# -gt 0 ]; do
    case "$1" in
        --profile)  profile="${2:?--profile wymaga nazwy}"; shift 2 ;;
        --out-dir)  out_dir="${2:?--out-dir wymaga sciezki}"; shift 2 ;;
        --image)    image="${2:?--image wymaga nazwy obrazu}"; shift 2 ;;
        --cpus)     cpus="${2:?--cpus wymaga liczby}"; shift 2 ;;
        --memory)   memory="${2:?--memory wymaga rozmiaru}"; shift 2 ;;
        --shm-size) shm_size="${2:?--shm-size wymaga rozmiaru}"; shift 2 ;;
        --pull)        force_pull=1; shift ;;
        --keep)        keep_container=1; shift ;;
        --reuse-build) reuse_build=1; shift ;;
        --reset-build) reset_build=1; reuse_build=1; shift ;;
        --list)     show_list; exit 0 ;;
        -h|--help)  show_help; exit 0 ;;
        *)
            echo "Nieznana opcja: $1" >&2
            echo "" >&2
            show_help >&2
            exit 1
            ;;
    esac
done

if ! printf '%s\n' "${profile_names[@]}" | grep -qx -- "$profile"; then
    echo "Nieznany profil: $profile" >&2
    echo "" >&2
    show_list >&2
    exit 1
fi

build_type="$(profile_build_type "$profile")"

# ── Wykrycie dzialajacego dockera ────────────────────────────────────────────
#
# Trzy osobne warunki z trzema osobnymi komunikatami, bo trzy rozne naprawy:
# brak klienta, zatrzymany demon, brak obrazu. Zaden nie ma sciezki zapasowej —
# build lokalny NIE jest odpowiedzia na "sprawdz to tak jak CI".
if ! command -v docker > /dev/null 2>&1; then
    echo "Blad: nie znaleziono polecenia 'docker'." >&2
    echo "Ten cel z zalozenia buduje w kontenerze takim jak CI; bez dockera nie ma czego uruchomic." >&2
    exit 1
fi

if ! docker info > /dev/null 2>&1; then
    echo "Blad: klient docker jest, ale demon nie odpowiada." >&2
    echo "Uruchom usluge dockera (Docker Desktop albo 'sudo systemctl start docker') i powtorz." >&2
    exit 1
fi

if [ "$force_pull" -eq 1 ]; then
    echo "-- Pobieram obraz $image"
    docker pull "$image"
elif ! docker image inspect "$image" > /dev/null 2>&1; then
    echo "-- Obrazu $image nie ma lokalnie, pobieram"
    if ! docker pull "$image"; then
        echo "Blad: nie udalo sie pobrac obrazu $image." >&2
        echo "Obraz buduje sie z docker/ci/Dockerfile (instrukcja na koncu tamtego pliku)." >&2
        exit 1
    fi
fi

# ── Kontener ─────────────────────────────────────────────────────────────────
container="rdb-test-ci-${profile}-$$"
work_dir="/home/developer/workspace/retractordb"

# BEZ nazwanego wolumenu na ccache, mimo ze config.yml ma pare
# ccache-restore / ccache-save. Powod jest pomiarowy: w tym drzewie ccache nie
# buforuje NICZEGO. CMake wlacza skanowanie modulow C++20 (CMAKE_CXX_STANDARD
# 23 + Ninja + GCC), wiec kazda linia kompilacji niesie `-fmodules-ts` i
# `-fmodule-mapper=`, a ccache 4.11 liczy to jako "Unsupported compiler
# option". Zmierzone na profilu smoke: 132 wywolania, 132 nieburzowalne, zero
# zapisow. Wolumen bylby wiec pustym katalogiem udajacym oszczednosc.
# Gdy ta przyczyna zniknie, wolumen ma sens i wraca tutaj.

# --reuse-build: katalog build/ w nazwanym wolumenie, osobnym dla kazdego
# profilu. Osobnym, bo profile roznia sie konfiguracja tego samego katalogu —
# `release` i `ablation-all-off` pisza oba do build/Release, ale z innymi
# RDB_OPT_*, i jeden wolumen dawalby drzewo skonfigurowane pod poprzedni
# przebieg.
#
# To jest ODSTEPSTWO od CI, ktore zawsze buduje z czystego checkoutu, i jego
# cena jest realna: zachowany katalog niesie CMakeCache, obiekty plikow, ktore
# w miedzyczasie zniknely, i skopiowane katalogi testow. Awaria widoczna
# wylacznie przy budowie od zera tu nie wyjdzie. Do sprawdzenia "czy przejdzie
# na CI" przed pushem — tak; jako dowod przed zgloszeniem czegokolwiek — nie.
build_volume="rdb-test-ci-build-${profile}"
run_opts=(--cpus "$cpus" --memory "$memory" --memory-swap "$memory" --shm-size "$shm_size")
if [ "$reuse_build" -eq 1 ]; then
    if [ "$reset_build" -eq 1 ]; then
        echo "-- Kasuje zachowany katalog build/ profilu $profile"
        docker volume rm "$build_volume" > /dev/null 2>&1 || true
    fi
    run_opts+=(--volume "$build_volume:$work_dir/build")
fi

cleanup() {
    if [ "$keep_container" -eq 1 ]; then
        echo "-- Kontener zostawiony: docker exec -it $container bash"
    else
        docker rm --force "$container" > /dev/null 2>&1 || true
    fi
}
trap cleanup EXIT

echo "== test-ci: profil $profile"
echo "   job CI:  $(profile_job "$profile")"
echo "   obraz:   $image"
echo "   zasoby:  --cpus $cpus --memory $memory --shm-size $shm_size"
echo "   wyniki:  $out_dir/$profile"
if [ "$reuse_build" -eq 1 ]; then
    echo "   build:   wolumen $build_volume — przebudowa inkrementalna, NIE jak czysty checkout CI"
fi

docker run --detach --name "$container" "${run_opts[@]}" \
    --workdir /home/developer/workspace \
    "$image" sleep infinity > /dev/null

# Punkt montowania wolumenu docker tworzy jako katalog roota, razem z brakujacym
# katalogiem nadrzednym — bez tego rozpakowanie drzewa przez uzytkownika
# `developer` konczy sie odmowa dostepu.
if [ "$reuse_build" -eq 1 ]; then
    docker exec --user root "$container" chown developer:developer "$work_dir" "$work_dir/build"
fi

# Testowana tresc: pliki sledzone + niesledzone nieignorowane, czyli drzewo
# roboczne tak, jakby zmiany byly zacommitowane. Ignorowane katalogi (build/,
# coverage/) nie jada — kontener ma budowac od zera, jak checkout w CI.
# --ignore-failed-read: `git ls-files -c` wypisuje takze pliki skasowane w
# drzewie roboczym, a tar nie ma ich juz czego przeczytac.
echo "-- Kopiuje drzewo robocze do kontenera"
docker exec "$container" mkdir -p "$work_dir"
(cd "$repo_root" && git ls-files -co --exclude-standard -z \
    | tar --null --files-from - --ignore-failed-read --create --file -) \
    | docker exec --interactive "$container" tar --extract --file - --directory "$work_dir"

# ── Kroki wewnatrz kontenera ─────────────────────────────────────────────────
#
# Heredoc w apostrofach: tresc idzie do kontenera doslownie, parametry przez
# argumenty pozycyjne. Kroki odpowiadaja komendom conan-install / conan-build /
# run-test z config.yml.
rc=0
docker exec --interactive "$container" bash -s -- "$profile" "$build_type" "$work_dir" << 'INSIDE' || rc=$?
set -o errexit
set -o nounset
set -o pipefail

profile="$1"
build_type="$2"
work_dir="$3"

cd "$work_dir"

# Obraz trzyma conana i cmake w ~/.venv (docker/ci/Dockerfile), a ~/.bashrc nie
# jest czytany przez powloke nieinteraktywna.
# shellcheck disable=SC1091
. "$HOME/.venv/bin/activate"
export PATH="$HOME/.local/bin:$PATH"
step() { echo; echo "== $*"; }

conan_install() {
    step "Conan install ($build_type)"
    conan source .
    conan install . -s build_type="$build_type" --build missing
}

conan_build() {
    step "Conan build ($build_type, ninja)"
    conan build . -s build_type="$build_type" --build missing
    (cd "build/$build_type" && ninja install)
}

smoke_test() {
    step "Smoke test"
    (cd "build/$build_type" && ninja showelf)
    xretractor -h
    xqry -h
    xtrdb -h
}

# ctest z `-j $(nproc)` doslownie jak w config.yml. W kontenerze `nproc` podaje
# rdzenie HOSTA, nie przydzial cgroup — dokladnie tak samo jak w CI, wiec i
# przesubskrybowanie jest to samo. Kod wyjscia wraca po zebraniu raportu.
run_tests() {
    step "Integration & Unit test"
    local rc=0
    (cd "build/$build_type/test" && ctest -j "$(nproc)" -V --output-junit test_results.xml) || rc=$?
    python3 scripts/collect-test-failures.py "build/$build_type" --ctest-status "$rc"
    return "$rc"
}

# Konfiguracja wariantu ablacyjnego: przelaczniki wprost, jak w jobie
# build-release-ablation. `--parallel 4` zamiast domyslnej rownoleglosci ninja
# (nproc+2) z tego samego powodu co w CI: jedna jednostka kompilacji tego
# drzewa siega ~1 GiB RSS, a kontener ma 8 GiB.
ablation_build() {
    local dedup="$1" share="$2" commutative="$3" factor="$4" simplify="$5" probe="$6"
    step "Configure ablation variant"
    cmake \
        -S . \
        -B build/Release \
        -G Ninja \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_TOOLCHAIN_FILE="$PWD/build/Release/generators/conan_toolchain.cmake" \
        -DRDB_OPT_DEDUP_SUBSTRATES="$dedup" \
        -DRDB_OPT_SHARE_EQUIVALENT_SELECTS="$share" \
        -DRDB_OPT_COMMUTATIVE_ADD="$commutative" \
        -DRDB_OPT_FACTOR_MATCHED_HASH_TIMEMOVES="$factor" \
        -DRDB_OPT_SIMPLIFY_EXPRESSIONS="$simplify" \
        -DRDB_BENCH_PROBE="$probe"

    step "Build and install ablation variant"
    cmake --build build/Release --parallel 4
    (cd build/Release && ninja -j 4 install)

    step "Verify compiled switches match the requested profile"
    local actual expected
    actual="$(xretractor --build-info)"
    expected="$(printf '%s\n' \
        "RDB_OPT_DEDUP_SUBSTRATES=$dedup" \
        "RDB_OPT_SHARE_EQUIVALENT_SELECTS=$share" \
        "RDB_OPT_COMMUTATIVE_ADD=$commutative" \
        "RDB_OPT_FACTOR_MATCHED_HASH_TIMEMOVES=$factor" \
        "RDB_BENCH_PROBE=$probe" \
        "RDB_OPT_SIMPLIFY_EXPRESSIONS=$simplify")"
    printf '%s\n' "$actual"
    if [ "$actual" != "$expected" ]; then
        echo "Error: built binary does not report the requested optimizer configuration." >&2
        printf 'expected:\n%s\n' "$expected" >&2
        exit 1
    fi
}

status=0
case "$profile" in
    smoke)
        conan_install
        conan_build
        ;;
    commit|release)
        conan_install
        conan_build
        smoke_test
        run_tests || status=$?
        ;;
    ablation-all-off)
        conan_install
        ablation_build OFF OFF OFF OFF OFF OFF
        run_tests || status=$?
        ;;
    ablation-probe-on)
        conan_install
        ablation_build ON ON ON ON ON ON
        run_tests || status=$?
        ;;
    gate)
        conan_install
        step "Configure Release tree (gate in strict mode)"
        cmake \
            -S . \
            -B build/Release \
            -G Ninja \
            -DCMAKE_BUILD_TYPE=Release \
            -DCMAKE_TOOLCHAIN_FILE="$PWD/build/Release/generators/conan_toolchain.cmake" \
            -DRESEARCH_GATE_STRICT=ON
        step "Build H9 ablation profiles"
        K6_CCACHE=1 K6_BUILD_JOBS=4 test/research_gate/h9/build_profiles.sh
        step "Research gate H9/H10"
        cmake --build build/Release --target test_gate --parallel 4
        ;;
esac

exit "$status"
INSIDE

# ── Wyniki ───────────────────────────────────────────────────────────────────
#
# Odpowiednik store_test_results / store_artifacts: to, co w CI zostaje po
# nieudanym jobie, ma zostac i tutaj — kontener zaraz znika.
profile_out="$out_dir/$profile"
mkdir -p "$profile_out"

copy_out() {
    docker cp "$container:$1" "$profile_out/" > /dev/null 2>&1 || true
}

copy_out "$work_dir/build/$build_type/test/test_results.xml"
copy_out "$work_dir/build/$build_type/test-failure-report"
if [ "$profile" = "gate" ]; then
    copy_out "$work_dir/build/research-gate-work"
    copy_out "$work_dir/build/gate-profiles-logs"
fi

echo
if [ "$rc" -eq 0 ]; then
    echo "== test-ci: profil $profile ZIELONY"
else
    echo "== test-ci: profil $profile CZERWONY (kod $rc)"
fi
echo "   wyniki: $profile_out"
exit "$rc"
