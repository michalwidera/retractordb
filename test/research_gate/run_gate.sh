#!/usr/bin/env bash
# Bramka badawcza — czy biezacy silnik nadal przechodzi bramki mechanizmowe
# hipotez H9 i H10.
#
# NIE jest to powtorzenie kampanii i NIE potwierdza hipotez. Patrz README.md,
# sekcja "Czego ta bramka nie sprawdza".
#
# Uzycie:
#   ./run_gate.sh --xretractor <sciezka> [--work <katalog>] [--count N]
#                 [--only h9|h10] [--profiles <katalog buildow ablacji>] [--strict]
#
# --strict: poziom POMINIETY konczy przebieg bledem. Pominiecie znaczy "nie
# uruchomiono", nigdy "zaliczono" — bez tej opcji przebieg bez profili ablacji
# konczy sie kodem 0 i w CI wygladalby na zaliczona bramke. Praca lokalna
# domyslnie pominiecie dopuszcza, CI nigdy.
#
# Pomocniczo, dla h9/build_profiles.sh:
#   ./run_gate.sh --print-src-fingerprint [--code-repo <sciezka>]
set -uo pipefail

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
CODE_REPO="$(cd "$HERE/../.." && pwd)"

# Nazwa pliku odcisku skladanego przez h9/build_profiles.sh w katalogu builda
# profilu. Uzgodniona miedzy tymi dwoma skryptami i nigdzie indziej.
STAMP=".gate-src-fingerprint"

# Odcisk TRESCI src/, a nie czasu modyfikacji. Git przepisuje mtime przy kazdym
# `checkout` i `pull --rebase`, wiec profil zbudowany z dokladnie tej samej
# tresci wygladal na nieswiezy i poziom 84/84 byl pomijany bez powodu
# (2026-08-18: profile z 17:00 uznane za nieswieze po rebase o 18:37, przy
# pustym `git diff -- src/`).
#
# Odcisk liczy sie z DRZEWA ROBOCZEGO (`ls-files -c -o` + `hash-object` po
# tresci plikow), nie z `HEAD:src`: niezacommitowana zmiana w src/ zmienia go
# tak samo jak commit, wiec profil zbudowany przed nia jest nieswiezy. Lista
# sciezek wchodzi do sumy razem z hashami, wiec dodanie i usuniecie pliku tez
# zmienia odcisk.
src_fingerprint() { # src_fingerprint <repozytorium> -> odcisk na stdout
  local repo="$1" list hashes
  list="$(git -C "$repo" ls-files -c -o --exclude-standard -- src 2>/dev/null | LC_ALL=C sort -u)" || return 1
  [[ -n "$list" ]] || return 1
  # Plik sledzony, ale usuniety z dysku, przewraca `hash-object` — i slusznie:
  # nieobliczalny odcisk ma znaczyc "nieswiezy", nigdy "swiezy".
  hashes="$(printf '%s\n' "$list" | git -C "$repo" hash-object --stdin-paths 2>/dev/null)" || return 1
  printf '%s\n%s\n' "$list" "$hashes" | sha256sum | awk '{print $1}'
}

XRETRACTOR=""
WORK="$HERE/.gate-work"
COUNT=10010
ONLY="both"
PROFILES=""
PRINT_FP=0
STRICT=0

while [[ $# -gt 0 ]]; do
  case "$1" in
    --xretractor) XRETRACTOR="$2"; shift 2 ;;
    --work)       WORK="$2"; shift 2 ;;
    --count)      COUNT="$2"; shift 2 ;;
    --only)       ONLY="$2"; shift 2 ;;
    --profiles)   PROFILES="$2"; shift 2 ;;
    --code-repo)  CODE_REPO="$(cd "$2" && pwd)" || exit 2; shift 2 ;;
    --print-src-fingerprint) PRINT_FP=1; shift ;;
    --strict)     STRICT=1; shift ;;
    *) echo "nieznany argument: $1" >&2; exit 2 ;;
  esac
done

# Tryb pomocniczy: sam odcisk, bez binarki i bez bramki. Wychodzi przed
# szukaniem xretractora, bo build_profiles.sh wola go ZANIM cokolwiek zbuduje.
if [[ "$PRINT_FP" -eq 1 ]]; then
  fp="$(src_fingerprint "$CODE_REPO")" || {
    echo "BLAD: nie udalo sie policzyc odcisku tresci src/ w $CODE_REPO" >&2
    exit 2
  }
  printf '%s\n' "$fp"
  exit 0
fi

if [[ -z "$XRETRACTOR" ]]; then
  for c in "$CODE_REPO/build/Debug/src/retractor/xretractor" \
           "$CODE_REPO/build/Release/src/retractor/xretractor"; do
    [[ -x "$c" ]] && { XRETRACTOR="$c"; break; }
  done
fi
if [[ ! -x "$XRETRACTOR" ]]; then
  echo "BLAD: nie znaleziono xretractor; podaj --xretractor <sciezka>" >&2
  exit 2
fi

mkdir -p "$WORK"
ENGINE_SHA="$(git -C "$CODE_REPO" rev-parse --short HEAD 2>/dev/null || echo nieznany)"
ENGINE_DIRTY="$(git -C "$CODE_REPO" status --short 2>/dev/null | wc -l)"

FAIL=0
SKIP=()
pass() { printf '  \033[32mPRZESZLA\033[0m  %s\n' "$1"; }
fail() { printf '  \033[31mOBLALA\033[0m    %s\n' "$1"; FAIL=1; }
skip() { printf '  POMINIETA %s\n' "$1"; SKIP+=("$2"); }

step() { # step <etykieta> <polecenie...>
  local label="$1"; shift
  if "$@" >"$WORK/$(echo "$label" | tr ' /' '__').log" 2>&1; then
    pass "$label"
  else
    fail "$label  (log: $WORK/$(echo "$label" | tr ' /' '__').log)"
  fi
}

check_h9_profiles() {
  # Wynik zostaje w zmiennych globalnych, aby wywolujacy mogl zdecydowac, czy
  # wolno przebudowac profile. Tylko rozjazd odcisku przy istniejacym profilu
  # oznacza "zbuduj ponownie"; brak profilu lub odcisku pozostaje pominieciem.
  H9_PROFILE_PROBLEM=""
  H9_PROFILE_REBUILD=0

  local cur_fp bin p stamp
  cur_fp="$(src_fingerprint "$CODE_REPO")" || cur_fp=""
  if [[ -z "$cur_fp" ]]; then
    H9_PROFILE_PROBLEM="nie udalo sie policzyc odcisku tresci src/"
    return 1
  fi

  for p in DEFAULT NO_R2_CANON NO_R1_FACTOR NO_R1_NO_R2; do
    bin="$PROFILES/K26v3-$p/src/retractor/xretractor"
    stamp="$PROFILES/K26v3-$p/$STAMP"
    if [[ ! -x "$bin" ]]; then
      H9_PROFILE_PROBLEM="brak profilu $p"
      return 1
    fi
    if [[ ! -f "$stamp" ]]; then
      H9_PROFILE_PROBLEM="profil $p bez odcisku zrodel (zbudowany starsza aparatura)"
      return 1
    fi
    if [[ "$(cat "$stamp" 2>/dev/null)" != "$cur_fp" ]]; then
      H9_PROFILE_PROBLEM="profil $p zbudowany z innej tresci src/"
      H9_PROFILE_REBUILD=1
      return 1
    fi
  done
}

echo "=============================================================="
echo " Bramka badawcza H9/H10"
echo "   silnik   : $ENGINE_SHA$([[ "$ENGINE_DIRTY" -gt 0 ]] && echo ' (drzewo BRUDNE)')"
echo "   binarka  : $XRETRACTOR"
echo "   katalog  : $WORK"
echo "=============================================================="

# ---------------------------------------------------------------- H10
if [[ "$ONLY" == "both" || "$ONLY" == "h10" ]]; then
  echo
  echo "-- H10: rachunek poczatku logicznego i ogona startowego --"
  cd "$HERE/h10"
  for t in test_independence test_oracle test_mutants; do
    step "H10 $t" python3 "tests/$t.py"
  done

  # test_closedform jako jedyny z tej czworki rozmawia z SILNIKIEM, wiec dostaje
  # binarke jawnie, jak test_phase_forms i kampanie nizej. Bez tego wpada w
  # resolve_binary(None) z oracle/engine.py: DEFAULT_BINARY wskazuje tam sciezke
  # z repozytorium eksperymentu, nieistniejaca w tym drzewie, wiec zostaje
  # fallback na `xretractor` z PATH. Na CI nie ma go wcale (job bramki nie robi
  # `ninja install`) i poziom oblewa; lokalnie jest, wiec poziom przechodzi —
  # tyle ze sprawdzajac ZAINSTALOWANA binarke, a nie te, ktora bada reszta bramki.
  step "H10 test_closedform" python3 tests/test_closedform.py "$XRETRACTOR"

  # Postacie fazowe `-`, `Theta` i `~Theta` (K24/H10, 2026-08-18). Osobny poziom,
  # bo korpus losowy siega q <= 5, a twierdzenie dotyczy kazdego q — przemiatanie
  # pyta silnik wprost dla q do 12. Poziom niesie wlasna kontrole mocy: jesli dawna
  # regula nie roznilaby sie od modelu w zadnym przypadku, konczy sie kodem 2.
  step "H10 test_phase_forms" python3 tests/test_phase_forms.py "$XRETRACTOR"

  # Dwa ziarna: w probie i poza proba. Kazde ma wlasny zamrozony werdykt
  # odniesienia, wiec kazde jest osobnym porownaniem.
  for pair in "20260804:VERDICT.md:w probie" "20260807:VERDICT_oos.md:poza proba"; do
    seed="${pair%%:*}"; rest="${pair#*:}"
    ref="${rest%%:*}"; label="${rest#*:}"

    step "H10 kampania $label ($COUNT planow, ziarno $seed)" python3 run_campaign.py \
        --seed "$seed" --count "$COUNT" --xretractor "$XRETRACTOR" \
        --out "$WORK/h10_$seed.csv"

    # Kampania przerwana w polowie zostawia CSV z samym naglowkiem. `-s` taki plik
    # przepuszcza, bo nie jest pusty, a `verdict.py` konczy sie na nim kodem 0
    # i zapisuje werdykt "0 planow, zero bledow aparatury" z pusta tabela rezimow.
    # Dopiero `compare_regimes.py` sie o to przewraca — kodem 2, wiec komunikat
    # bramki wskazywal na odczyt werdyktu zamiast na kampanie, a w katalogu
    # dowodowym zostawal werdykt orzekajacy o zerze obserwacji. Warunkiem jest
    # wiersz obserwacji, nie niepusty plik.
    rows="$(tail -n +2 "$WORK/h10_$seed.csv" 2>/dev/null | grep -c .)"
    if [[ "${rows:-0}" -lt 1 ]]; then
      fail "H10 kampania $label — CSV bez ani jednego wiersza obserwacji (sam naglowek)"
      continue
    fi

    step "H10 werdykt $label" python3 verdict.py --raw "$WORK/h10_$seed.csv" \
        --out "$WORK/h10_VERDICT_$seed.md" --seed "$seed" --engine "$ENGINE_SHA"
    # Brak werdyktu przerywa te iteracje, ale nie wolno mu jej WYCISZYC: samo
    # `continue` kasowalo poziom "H10 rezimy" z listy, wiec przebieg konczyl sie
    # bez ani jednego sladu po nim — ani oblany, ani pominiety. Werdykt okrojony,
    # a nie brakujacy, lapie dalej `compare_regimes.py` kodem 2.
    if [[ ! -s "$WORK/h10_VERDICT_$seed.md" ]]; then
      fail "H10 rezimy $label — brak werdyktu do porownania"
      continue
    fi

    # Jadro bramki. Porownanie jest KIERUNKOWE: poprawa dokladnosci nie
    # zatrzymuje pracy, utrata dokladnosci zatrzymuje.
    out="$(python3 "$HERE/compare_regimes.py" --reference "$ref" \
             --current "$WORK/h10_VERDICT_$seed.md" 2>&1)"
    rc=$?
    printf '%s\n' "$out" | sed 's/^/    /' | tee "$WORK/h10_regimes_$seed.txt" >/dev/null
    printf '%s\n' "$out" | grep -E 'POPRAWA|REGRESJA|DEFEKT|BLAD|dokladne' | sed 's/^ */        /'
    case "$rc" in
      0) if printf '%s' "$out" | grep -q POPRAWA; then
           pass "H10 rezimy $label — bez regresji, z POPRAWA (kierunek rozwoju)"
         else
           pass "H10 rezimy $label zgodne z odniesieniem"
         fi ;;
      1) fail "H10 rezimy $label — REGRESJA dokladnosci" ;;
      *) fail "H10 rezimy $label — blad odczytu werdyktu (kod $rc)" ;;
    esac
  done
fi

# ----------------------------------------------------------------- H9
if [[ "$ONLY" == "both" || "$ONLY" == "h9" ]]; then
  echo
  echo "-- H9: mechanizm wspoldzielenia materializowanego podplanu --"
  cd "$HERE/h9"
  step "H9 korpus zgodny z generatorem"  python3 gen_corpus.py --check
  step "H9 samotest bramki korpusu"      python3 validate_corpus.py --selftest
  step "H9 samotest procedury decyzyjnej" python3 verdict.py --selftest
  step "H9 tablica mechanizmu (znana odpowiedz)" python3 mechanism_table.py --gate

  # Poziom 2 — 84 kompilacje na czterech profilach ablacji. Wymaga zbudowanych
  # profili, bo bez nich nie da sie odroznic R1 od R2.
  if [[ -n "$PROFILES" ]]; then
    # Binarka nie niesie SHA zrodla (`--build-info` podaje tylko flagi), wiec
    # swiezosc rozstrzyga odcisk tresci src/ zapisany przy budowie profilu
    # (build_profiles.sh -> $STAMP). Profil zbudowany z innej tresci orzekalby
    # o innej rewizji niz badana — to falszywa zielen. Kierunek bledu jest
    # jednostronny: cokolwiek nie da sie potwierdzic, jest NIESWIEZE.
    h9_profiles_ready=0
    if check_h9_profiles; then
      h9_profiles_ready=1
    elif [[ "$H9_PROFILE_REBUILD" -eq 1 ]]; then
      echo "  ODTWARZANIE H9 profili — $H9_PROFILE_PROBLEM"
      if RDB_CODE_REPO="$CODE_REPO" "$HERE/h9/build_profiles.sh"; then
        if check_h9_profiles; then
          h9_profiles_ready=1
        else
          fail "H9 profile po przebudowie — $H9_PROFILE_PROBLEM"
        fi
      else
        fail "H9 przebudowa profili — $H9_PROFILE_PROBLEM"
      fi
    else
      skip "H9 84/84 kompilacji — $H9_PROFILE_PROBLEM" "h9-profile-stale"
      echo "        Wymaga: $HERE/h9/build_profiles.sh"
    fi

    if [[ "$h9_profiles_ready" -eq 1 ]]; then
      # Na brudnym drzewie poziom nadal sie wykonuje — bramka chroni rozwoj,
      # a podczas rozwoju drzewo jest brudne. Dowod dostaje wtedy SHA z sufiksem
      # `-dirty` i nie jest dowodem proweniencji.
      dirty_args=()
      [[ "$ENGINE_DIRTY" -gt 0 ]] && dirty_args=(--allow-dirty)

      # `validate_corpus.py` z zasady nie nadpisuje katalogu dowodowego i ta
      # odmowa ma zostac: dowod kampanii nie moze zniknac pod kolejnym
      # przebiegiem. Tyle ze bramka podaje jej stala sciezke w `$WORK`, czyli
      # w katalogu roboczym w drzewie buildu, wiec drugi `ninja test_gate` z rzedu
      # oblewal na aparaturze, nie na silniku — a przy pracy nad `src/` drugi
      # przebieg jest regula, nie wyjatkiem. Bramka sprzata WLASNY poprzedni
      # dowod, rozpoznany po jego wlasnym ukladzie (manifest + tabela wynikow).
      # Katalog o innej zawartosci nie jest kasowany: to albo cudzy dowod, albo
      # przerwany przebieg, i decyzje podejmuje czlowiek.
      evidence="$WORK/h9_corpus_validation"
      evidence_ready=1
      if [[ -d "$evidence" ]]; then
        if [[ -f "$evidence/manifest.sha256" && -f "$evidence/corpus-validation.tsv" ]]; then
          rm -rf "$evidence"
        else
          evidence_ready=0
          fail "H9 84/84 kompilacji — $evidence istnieje i nie ma ukladu dowodu tej bramki"
          echo "        Obejrzyj go i usun recznie, jesli jest do wyrzucenia."
        fi
      fi

      if [[ "$evidence_ready" -eq 1 ]]; then
        step "H9 84/84 kompilacji + 4/4 odrzucone mutanty$([[ "$ENGINE_DIRTY" -gt 0 ]] && echo ' (drzewo brudne)')" \
          env K26V3_BUILD_ROOT="$PROFILES" python3 validate_corpus.py \
              --out "$evidence" "${dirty_args[@]}"
      fi
    fi
  else
    skip "H9 84/84 kompilacji na czterech profilach ablacji" "h9-profile"
    echo "        Wymaga --profiles <katalog>; zbuduj: h9/build_profiles.sh"
  fi
fi

# -------------------------------------------------------------- podsumowanie
echo
echo "=============================================================="
# Pominiety poziom nie jest zaliczony. Bez --strict przebieg tylko go odnotowuje;
# z --strict oblewa, bo zielona bramka o niepelnym zakresie jest gorsza od jej braku.
if [[ "$STRICT" -eq 1 && "${#SKIP[@]}" -gt 0 ]]; then
  FAIL=1
  echo " STRICT: poziom pominiety liczy sie jako OBLANY (${#SKIP[@]}: ${SKIP[*]})"
fi
if [[ "$FAIL" -eq 0 ]]; then
  echo " WYNIK: bramki mechanizmowe PRZESZLY na silniku $ENGINE_SHA"
else
  echo " WYNIK: BRAMKA OBLALA na silniku $ENGINE_SHA"
fi
if [[ "${#SKIP[@]}" -gt 0 ]]; then
  echo " POMINIETO: ${SKIP[*]}"
fi
cat <<'EOF'

 Czego ta bramka NIE sprawdzila, niezaleznie od wyniku:
   * progu czasowego H9 (redukcja >=40%, gorna granica CI <=1,05) — wymaga
     1440 komorek na przypietym pi400 pod PREEMPT_RT, ok. 48 h;
   * bramki oracle_values H9 wobec Flinka — wymaga Flink 2.3.0 i JDK 17;
   * niczego, co czynilo kampanie dowodem: predeklaracji, zamrozenia,
     jednokrotnego uruchomienia procedury decyzyjnej.
 Zielona bramka znaczy "mechanizm nadal dziala tak samo", nie "hipoteza
 potwierdzona na tym silniku".
EOF
echo "=============================================================="
exit "$FAIL"
