#!/usr/bin/env bash
# Bramka badawcza — czy biezacy silnik nadal przechodzi bramki mechanizmowe
# hipotez H9 i H10.
#
# NIE jest to powtorzenie kampanii i NIE potwierdza hipotez. Patrz README.md,
# sekcja "Czego ta bramka nie sprawdza".
#
# Uzycie:
#   ./run_gate.sh --xretractor <sciezka> [--work <katalog>] [--count N]
#                 [--only h9|h10] [--profiles <katalog buildow ablacji>]
set -uo pipefail

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
CODE_REPO="$(cd "$HERE/../.." && pwd)"

XRETRACTOR=""
WORK="$HERE/.gate-work"
COUNT=10010
ONLY="both"
PROFILES=""

while [[ $# -gt 0 ]]; do
  case "$1" in
    --xretractor) XRETRACTOR="$2"; shift 2 ;;
    --work)       WORK="$2"; shift 2 ;;
    --count)      COUNT="$2"; shift 2 ;;
    --only)       ONLY="$2"; shift 2 ;;
    --profiles)   PROFILES="$2"; shift 2 ;;
    *) echo "nieznany argument: $1" >&2; exit 2 ;;
  esac
done

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
  for t in test_independence test_oracle test_mutants test_closedform; do
    step "H10 $t" python3 "tests/$t.py"
  done

  # Dwa ziarna: w probie i poza proba. Kazde ma wlasny zamrozony werdykt
  # odniesienia, wiec kazde jest osobnym porownaniem.
  for pair in "20260804:VERDICT.md:w probie" "20260807:VERDICT_oos.md:poza proba"; do
    seed="${pair%%:*}"; rest="${pair#*:}"
    ref="${rest%%:*}"; label="${rest#*:}"

    step "H10 kampania $label ($COUNT planow, ziarno $seed)" python3 run_campaign.py \
        --seed "$seed" --count "$COUNT" --xretractor "$XRETRACTOR" \
        --out "$WORK/h10_$seed.csv"
    [[ -s "$WORK/h10_$seed.csv" ]] || continue

    step "H10 werdykt $label" python3 verdict.py --raw "$WORK/h10_$seed.csv" \
        --out "$WORK/h10_VERDICT_$seed.md" --seed "$seed" --engine "$ENGINE_SHA"
    [[ -s "$WORK/h10_VERDICT_$seed.md" ]] || continue

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
    # jedyny dostepny test swiezosci to czas modyfikacji. Profil starszy od
    # zrodel orzekalby o innej rewizji niz badana — to falszywa zielen.
    stale=""
    for p in DEFAULT NO_R2_CANON NO_R1_FACTOR NO_R1_NO_R2; do
      bin="$PROFILES/K26v3-$p/src/retractor/xretractor"
      if [[ ! -x "$bin" ]]; then stale="brak profilu $p"; break; fi
      if [[ -n "$(find "$CODE_REPO/src" -type f -newer "$bin" -print -quit 2>/dev/null)" ]]; then
        stale="profil $p starszy od zrodel w src/"; break
      fi
    done

    if [[ -n "$stale" ]]; then
      skip "H9 84/84 kompilacji — $stale" "h9-profile-stale"
      echo "        Przebudowac: $HERE/h9/build_profiles.sh"
    else
      # Na brudnym drzewie poziom nadal sie wykonuje — bramka chroni rozwoj,
      # a podczas rozwoju drzewo jest brudne. Dowod dostaje wtedy SHA z sufiksem
      # `-dirty` i nie jest dowodem proweniencji.
      dirty_args=()
      [[ "$ENGINE_DIRTY" -gt 0 ]] && dirty_args=(--allow-dirty)
      step "H9 84/84 kompilacji + 4/4 odrzucone mutanty$([[ "$ENGINE_DIRTY" -gt 0 ]] && echo ' (drzewo brudne)')" \
        env K26V3_BUILD_ROOT="$PROFILES" python3 validate_corpus.py \
            --out "$WORK/h9_corpus_validation" "${dirty_args[@]}"
    fi
  else
    skip "H9 84/84 kompilacji na czterech profilach ablacji" "h9-profile"
    echo "        Wymaga --profiles <katalog>; zbuduj: h9/build_profiles.sh"
  fi
fi

# -------------------------------------------------------------- podsumowanie
echo
echo "=============================================================="
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
