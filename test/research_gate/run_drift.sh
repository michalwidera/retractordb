#!/usr/bin/env bash
# Fizyczna weryfikacja braku dryftu silnika wzgledem wynikow H9 i H10.
#
# Czym to sie rozni od `run_gate.sh`
# ----------------------------------
# `run_gate.sh` porownuje ETYKIETY: bierze dwa zamrozone ziarna, sprowadza wynik
# do etykiety rezimu per klasa i zestawia ja z tablica odniesienia. Trzy rodzaje
# dryftu sa dla niej niewidoczne Z KONSTRUKCJI:
#
#   1. dryft WARTOSCI — bramka nie sprawdza, co silnik policzyl. Zle liczby przy
#      nietknietych rezimach i nietknietych kompilacjach przechodza na zielono;
#   2. dryft POZA KORPUSEM dwoch zamrozonych ziaren — zawsze te same 2 x 10 010
#      planow, wiec zmiana zachowania poza nimi nie ma jak sie ujawnic;
#   3. dryft WIELKOSCI wewnatrz etykiety — klasa juz zawyzajaca moze zawyzac
#      coraz mocniej, a etykieta sie nie zmieni (dzis uspiony: wszystkie dziewiec
#      klas jest dokladnych, gdzie etykieta jest ciasna).
#
# Ten skrypt WYKONUJE silnik i konfrontuje go z czyms niezaleznym od niego:
#
#   1. losuje ZIARNO w chwili uruchomienia — kazdy przebieg przemiata inny
#      wycinek przestrzeni planow generatora, zamiast wracac w to samo miejsce;
#   2. zestawia wynik z MODELEM ZDARZENIOWYM (H10) i z NIEZALEZNA IMPLEMENTACJA
#      w Apache Flink (H9, wartosci), a nie z tablica odniesienia;
#   3. dopisuje kazdy przebieg do `DRIFT_JOURNAL.tsv`, takze przebieg z dryftem,
#      zeby bylo wiadomo, ktora rewizja silnika byla fizycznie sprawdzona i z
#      jakim wynikiem.
#
# NIE jest to powtorzenie kampanii i nie produkuje wpisu do repozytorium
# eksperymentu. Dokumentem obowiazujacym dla artykulu pozostaja kampanie K24d
# i K26v3; ten skrypt pilnuje, zeby silnik od nich nie odjechal.
#
# Poziomy sa TROJWARTOSCIOWE: ZGODNY / DRYFT / NIESPRAWDZONY.
# Poziom niesprawdzony nie jest zaliczony i nigdy nie jest przemilczany.
#
# Uzycie:
#   ./run_drift.sh --xretractor <sciezka> [--work <katalog>] [--count N]
#                  [--seed N] [--only h9|h10] [--profiles <katalog>]
#                  [--no-values] [--journal <plik>] [--strict]
set -uo pipefail

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
CODE_REPO="$(cd "$HERE/../.." && pwd)"
JOURNAL="$HERE/DRIFT_JOURNAL.tsv"

# Ziarna kampanii, na ktorych hipoteza H10 zostala postawiona. Przebieg na nich
# nie jest proba spoza — skrypt na to pozwala (odtworzenie wpisu z dziennika),
# ale mowi o tym glosno.
FROZEN_SEEDS=("20260803" "20260804" "20260806" "20260807")

# Wolanie zapamietane PRZED petla parsujaca — ta zjada argumenty `shift`-em,
# wiec pozniej `$*` jest juz puste, a opis w blokadzie ma nazywac przebieg,
# ktory ja trzyma.
INVOCATION="$0 $*"

XRETRACTOR=""
WORK=""
COUNT=10010
SEED=""
ONLY="both"
PROFILES=""
WANT_VALUES=1
STRICT=0

while [[ $# -gt 0 ]]; do
  case "$1" in
    --xretractor) XRETRACTOR="$2"; shift 2 ;;
    --work)       WORK="$2"; shift 2 ;;
    --count)      COUNT="$2"; shift 2 ;;
    --seed)       SEED="$2"; shift 2 ;;
    --only)       ONLY="$2"; shift 2 ;;
    --profiles)   PROFILES="$2"; shift 2 ;;
    --no-values)  WANT_VALUES=0; shift ;;
    --journal)    JOURNAL="$2"; shift 2 ;;
    --strict)     STRICT=1; shift ;;
    *) echo "nieznany argument: $1" >&2; exit 2 ;;
  esac
done

WORK="${WORK:-$HERE/.drift-work}"

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
# Sciezka bezwzgledna, bo poziomy uruchamiaja sie po `cd` do katalogu hipotezy;
# sciezka wzgledna wygladalaby tam na brak binarki, czyli na awarie aparatury.
XRETRACTOR="$(readlink -f "$XRETRACTOR")"

# Ziarno losowane z /dev/urandom, nie z zegara: przebieg wykonany dwa razy w tej
# samej sekundzie ma dostac dwie rozne proby.
if [[ -z "$SEED" ]]; then
  SEED="$(od -An -N4 -tu4 </dev/urandom | tr -d ' ')"
  SEED_ORIGIN="wylosowane"
else
  SEED_ORIGIN="podane"
  for frozen in "${FROZEN_SEEDS[@]}"; do
    [[ "$SEED" == "$frozen" ]] && SEED_ORIGIN="podane, W PROBIE kampanii"
  done
fi

# Wylacznosc przebiegu. Poziomy H9 pisza i czytaja DOWOD KOMPILACJI pod stala
# sciezka w drzewie zrodlowym — `h9/corpus_validation` — bo tam, i tylko tam,
# szuka go `run_gates.py` (przez `validate_corpus.HERE`). Dwa przebiegi naraz
# nadpisalyby sobie ten dowod: jeden kasowalby katalog w chwili, gdy drugi go
# czyta, i bramka `corpus_validity` orzekalaby o cudzych plikach albo o pustce.
#
# `flock` zwalnia sie z zamknieciem deskryptora, wiec przebieg ubity albo
# przerwany nie zostawia blokady do recznego sprzatania. Deskryptor otwierany
# jest w trybie DOPISYWANIA: `>` obcinaloby plik przy samym otwarciu, czyli
# jeszcze przed proba zajecia blokady — drugi przebieg kasowalby wtedy opis
# pierwszemu, zanim dowie sie, ze ma sie wycofac.
LOCK="$HERE/.drift.lock"
if ! command -v flock >/dev/null 2>&1; then
  echo "BLAD: brak flock — bez niego nie da sie zapewnic wylacznosci przebiegu" >&2
  exit 2
fi
exec 9>>"$LOCK" || { echo "BLAD: nie da sie otworzyc $LOCK" >&2; exit 2; }
if ! flock -n 9; then
  echo "BLAD: inny przebieg weryfikacji dryftu juz biegnie." >&2
  echo "  Trzyma: $(tail -1 "$LOCK" 2>/dev/null || echo 'nieznany')" >&2
  echo "  Rownolegle przebiegi nadpisalyby sobie dowod w h9/corpus_validation." >&2
  exit 2
fi
: >"$LOCK"
printf 'pid=%s start=%s cmd=%s\n' "$$" "$(date -Is)" "$INVOCATION" >>"$LOCK"

mkdir -p "$WORK"
ENGINE_SHA="$(git -C "$CODE_REPO" rev-parse --short HEAD 2>/dev/null || echo nieznany)"
# Dziennik lezy w drzewie zrodlowym i sam siebie brudzi; nie moze wchodzic do
# oceny czystosci drzewa, ktora ten sam przebieg do niego wpisuje.
DIRTY_COUNT="$(git -C "$CODE_REPO" status --short 2>/dev/null | grep -vc 'DRIFT_JOURNAL.tsv')"
TREE="czyste"
[[ "${DIRTY_COUNT:-0}" -gt 0 ]] && TREE="BRUDNE"

DRIFTED=0
CHECKED=0
UNCHECKED=()
S_H10A="nieuruchomiony"; S_H10B="nieuruchomiony"
S_MECH="nieuruchomiony"; S_VALUES="nieuruchomiony"; S_TIMING="nieuruchomiony"

confirm()   { printf '  \033[32mZGODNY\033[0m         %s\n' "$1"; CHECKED=$((CHECKED + 1)); }
refute()    { printf '  \033[31mDRYFT\033[0m          %s\n' "$1"; DRIFTED=1; }
undecided() { printf '  \033[33mNIESPRAWDZONY\033[0m  %s\n' "$1"; UNCHECKED+=("$2"); }

log_of() { printf '%s/%s.log' "$WORK" "$(echo "$1" | tr ' /=' '___')"; }

# Uruchomienie kroku POMOCNICZEGO: jego porazka nie jest wynikiem o hipotezie,
# tylko awaria aparatury, wiec konczy poziom jako NIESPRAWDZONY.
aux() { # aux <etykieta> <polecenie...>
  local label="$1"; shift
  if "$@" >"$(log_of "$label")" 2>&1; then
    printf '  .            %s\n' "$label"
    return 0
  fi
  printf '  .            %s — AWARIA APARATURY (log: %s)\n' "$label" "$(log_of "$label")"
  return 1
}

echo "=============================================================="
echo " Fizyczna weryfikacja braku dryftu silnika wzgledem wynikow H9/H10"
echo "   silnik   : $ENGINE_SHA, drzewo $TREE"
echo "   binarka  : $XRETRACTOR"
echo "   ziarno   : $SEED ($SEED_ORIGIN)"
echo "   katalog  : $WORK"
echo "=============================================================="

# Ile razy ta sama rewizja byla juz sprawdzana. Nie jest to zakaz — kazdy
# przebieg bierze inne ziarno, wiec powtorzenie poszerza pokrycie. Jest to
# informacja: dziennik ma pokazywac, ktora rewizja byla sprawdzona i z jakim
# wynikiem, wiec warto wiedziec, ze wpisow o tej rewizji bedzie kilka.
if [[ -f "$JOURNAL" ]]; then
  PRIOR="$(awk -F'\t' -v sha="$ENGINE_SHA" '$2 == sha' "$JOURNAL" | wc -l)"
  if [[ "${PRIOR:-0}" -gt 0 ]]; then
    echo
    echo "  UWAGA: rewizja $ENGINE_SHA byla juz sprawdzana $PRIOR raz(y) — patrz $JOURNAL."
    echo "  Kolejny przebieg bierze inne ziarno, wiec poszerza pokrycie, a nie powtarza"
    echo "  tego samego sprawdzenia."
  fi
fi

# Czy jedyna porazka bramek P6 jest proweniencja na brudnym drzewie?
#
# Odpowiedz TAK wymaga trzech rzeczy naraz, bo kazda z osobna daje sie spelnic
# przypadkiem: drzewo musi byc brudne, jedyna bramka ze statusem FAIL musi byc
# `corpus_validity`, a kazda linia rozbieznosci wypisana w sekcji STOP-6 musi byc
# tym samym SHA po obu stronach, roznym tylko sufiksem `-dirty`. Cokolwiek innego
# w tej sekcji — inne SHA, inna bramka, dodatkowa linia — konczy sie NIE.
only_provenance_failed() { # only_provenance_failed <gates.tsv> <log>
  local gates="$1" log="$2" failed
  [[ "$TREE" == "BRUDNE" ]] || return 1
  [[ -f "$gates" && -f "$log" ]] || return 1
  failed="$(awk -F'\t' '$3 == "FAIL" { print $2 }' "$gates" | sort -u | tr '\n' ' ')"
  [[ "$failed" == "corpus_validity " ]] || return 1
  # Porownanie polami, nie wyrazeniem z odwolaniem wstecznym: odwolan wstecznych
  # nie ma w ERE w ogole, a przedzialow {n} nie zna mawk, ktory bywa `awk`-iem
  # domyslnym. Ta postac dziala tak samo pod gawk i pod mawk.
  awk '
    /^== STOP-6:/ { inside = 1; next }
    inside && /^   / {
      seen++
      if (NF != 6 || $1 != "engine" || $2 != "SHA" ||
          $4 != "evidence/start" || $5 != "pin") { bad++; next }
      left = $3; sub(/,$/, "", left)
      right = $6
      if (right !~ /-dirty$/) { bad++; next }
      sub(/-dirty$/, "", right)
      if (left != right || length(left) != 40 || left !~ /^[0-9a-f]+$/) bad++
    }
    END { exit (seen > 0 && bad == 0) ? 0 : 1 }
  ' "$log"
}

# ---------------------------------------------------------------- H10
if [[ "$ONLY" == "both" || "$ONLY" == "h10" ]]; then
  echo
  echo "-- H10: rachunek poczatku logicznego i ogona startowego --"
  cd "$HERE/h10" || exit 2

  # Aparatura orzeka o silniku tylko wtedy, gdy sama jest sprawdzona. Kolejnosc
  # jest czescia procedury: model zdarzeniowy, ktory nie odrzuca wlasnych
  # mutantow, „potwierdza" wszystko i nie potwierdza niczego.
  h10_ready=1
  for t in test_independence test_oracle test_mutants; do
    aux "H10 aparatura $t" python3 "tests/$t.py" || h10_ready=0
  done
  aux "H10 aparatura test_closedform" python3 tests/test_closedform.py "$XRETRACTOR" || h10_ready=0
  aux "H10 aparatura test_phase_forms" python3 tests/test_phase_forms.py "$XRETRACTOR" || h10_ready=0
  aux "H10 samotest reguly decyzyjnej" python3 decision_rule.py --selftest || h10_ready=0

  if [[ "$h10_ready" -eq 0 ]]; then
    undecided "H10 — aparatura nie przeszla wlasnych kontroli, wynik o silniku nie powstaje" "h10-aparatura"
    S_H10A="NIESPRAWDZONY"; S_H10B="NIESPRAWDZONY"
  else
    raw="$WORK/h10_$SEED.csv"
    if ! aux "H10 kampania ($COUNT planow, ziarno $SEED)" python3 run_campaign.py \
             --seed "$SEED" --count "$COUNT" --xretractor "$XRETRACTOR" --out "$raw"; then
      undecided "H10 — kampania nie doszla do konca" "h10-kampania"
      S_H10A="NIESPRAWDZONY"; S_H10B="NIESPRAWDZONY"
    else
      out="$WORK/H10_DRIFT_$SEED.md"
      python3 decision_rule.py --raw "$raw" --out "$out" \
              --seed "$SEED" --engine "$ENGINE_SHA" >"$WORK/h10_rule.txt" 2>&1
      rc=$?
      sed 's/^/        /' "$WORK/h10_rule.txt"
      S_H10B="$(sed -n 's/^H10b: \(.*\) — .*/\1/p' "$WORK/h10_rule.txt" | head -1)"
      S_H10B="${S_H10B:-nieznany}"
      case "$rc" in
        0) confirm "H10a na swiezej probie (ziarno $SEED, werdykt: $out)"
           S_H10A="ZGODNY" ;;
        1) refute  "H10a na swiezej probie (ziarno $SEED, werdykt: $out)"
           S_H10A="DRYFT" ;;
        *) undecided "H10a — regula decyzyjna odmowila werdyktu (kod $rc)" "h10-werdykt"
           S_H10A="NIESPRAWDZONY" ;;
      esac
      # Czlon (b) ma wlasny status i nie chowa sie w statusie czlonu (a).
      if [[ "$S_H10B" == "NIEOCENIALNY" ]]; then
        undecided "H10b — nieocenialny na tej aparaturze (zlamana kontrola negatywna)" "h10b"
      fi
    fi
  fi
fi

# ----------------------------------------------------------------- H9
if [[ "$ONLY" == "both" || "$ONLY" == "h9" ]]; then
  echo
  echo "-- H9: mechanizm wspoldzielenia materializowanego podplanu --"
  cd "$HERE/h9" || exit 2

  # H9 nie ma odpowiednika swiezego ziarna: korpus 21 planow jest PREDEKLAROWANY
  # i zamrozony. To nie jest slabosc — korpus powstal przed ta rewizja silnika,
  # wiec przebieg na nowym silniku jest potwierdzeniem, dopoki korpusu sie nie
  # rusza. Pierwszy poziom sprawdza wlasnie to.
  mech_ready=1
  aux "H9 korpus zgodny z generatorem"       python3 gen_corpus.py --check || mech_ready=0
  aux "H9 samotest bramki korpusu"           python3 validate_corpus.py --selftest || mech_ready=0
  aux "H9 samotest procedury decyzyjnej"     python3 verdict.py --selftest || mech_ready=0
  aux "H9 tablica mechanizmu (znana odpowiedz)" python3 mechanism_table.py --gate || mech_ready=0

  if [[ "$mech_ready" -eq 0 ]]; then
    undecided "H9 mechanizm — aparatura nie przeszla wlasnych kontroli" "h9-aparatura"
    S_MECH="NIESPRAWDZONY"
  elif [[ -z "$PROFILES" ]]; then
    undecided "H9 mechanizm — bez profili ablacji nie da sie odroznic R1 od R2" "h9-profile"
    S_MECH="NIESPRAWDZONY"
  else
    # Dowod kompilacji laduje tam, gdzie szuka go `run_gates.py`
    # (`validate_corpus.HERE/corpus_validation`); sciezka jest w .gitignore.
    #
    # Blokada wylacznosci wyklucza drugi PRZEBIEG, ale nie wyklucza katalogu
    # zostawionego przez przebieg przerwany albo przez recznie uruchomione
    # `validate_corpus.py`. Kasujemy wiec tylko katalog o WLASNYM ukladzie
    # (manifest + tabela wynikow); katalog o innej zawartosci jest cudzy albo
    # niepelny i decyzje o nim podejmuje czlowiek.
    evidence="$HERE/h9/corpus_validation"
    evidence_ready=1
    if [[ -d "$evidence" ]]; then
      if [[ -f "$evidence/manifest.sha256" && -f "$evidence/corpus-validation.tsv" ]]; then
        rm -rf "$evidence"
      else
        evidence_ready=0
        undecided "H9 mechanizm — $evidence istnieje i nie ma ukladu dowodu tej aparatury" "h9-dowod"
        echo "        Obejrzyj go i usun recznie, jesli jest do wyrzucenia."
        S_MECH="NIESPRAWDZONY"
      fi
    fi
    dirty_args=()
    [[ "$TREE" == "BRUDNE" ]] && dirty_args=(--allow-dirty)
    if [[ "$evidence_ready" -eq 0 ]]; then
      : # poziom juz odnotowany jako niesprawdzony
    elif K26V3_BUILD_ROOT="$PROFILES" python3 validate_corpus.py --out "$evidence" \
         "${dirty_args[@]}" >"$(log_of "H9 84 kompilacje")" 2>&1; then
      confirm "H9 mechanizm — 84/84 kompilacji na czterech profilach, 4/4 odrzucone mutanty"
      S_MECH="ZGODNY"
    else
      # Rozjazd tablicy mechanizmu jest wynikiem o silniku, nie awaria: znaczy,
      # ze rozpoznawanie rownowaznych obliczen dziala inaczej niz w kampanii.
      refute "H9 mechanizm — 84/84 kompilacji (log: $(log_of "H9 84 kompilacje"))"
      S_MECH="DRYFT"
    fi
  fi

  # ---- poziom wartosci: RetractorDB wobec niezaleznego portu w Apache Flink
  JAVA_PIN="${JAVA_HOME_PINNED:-/usr/lib/jvm/java-17-openjdk-amd64}"
  FLINK_PIN="${FLINK_HOME:-$HOME/opt/flink-2.3.0}"
  XTRDB_PIN="${XTRDB:-$HOME/.local/bin/xtrdb}"
  values_missing=""
  [[ -x "$JAVA_PIN/bin/java" ]]          || values_missing+=" JDK17($JAVA_PIN)"
  [[ -d "$FLINK_PIN/lib" ]]              || values_missing+=" Flink($FLINK_PIN)"
  [[ -f "$HERE/h9/flink/build/F9R2Job.class" ]] || values_missing+=" klasy-Flinka"
  [[ -x "$XTRDB_PIN" ]]                  || values_missing+=" xtrdb($XTRDB_PIN)"
  [[ "$S_MECH" == "ZGODNY" ]]      || values_missing+=" dowod-84/84"

  if [[ "$WANT_VALUES" -eq 0 ]]; then
    undecided "H9 wartosci wobec Flinka — pominiete na zadanie (--no-values)" "h9-wartosci"
    S_VALUES="NIESPRAWDZONY"
  elif [[ -n "$values_missing" ]]; then
    undecided "H9 wartosci wobec Flinka — brak:$values_missing" "h9-wartosci"
    S_VALUES="NIESPRAWDZONY"
    echo "        Zaleznosci instaluje: scripts/buildrdb.sh gate_requirements"
  else
    echo "        Poziom wartosci: 84 przebiegi RetractorDB + 36 jobow Flinka, kilkanascie minut."
    values_ready=1
    aux "H9 zrzuty planow kontrolnych" \
        env CODE_REPO="$CODE_REPO" OUT="$WORK/h9_plans" ./dump_control_plans.sh || values_ready=0
    aux "H9 przebiegi RetractorDB (dane glowne, cztery profile)" \
        env CODE_REPO="$CODE_REPO" OUT="$WORK/h9_rdb" ./run_main_rdb.sh || values_ready=0
    aux "H9 przebiegi Flinka (3 rodziny x 6 Q x 2 warianty)" \
        env JAVA_HOME_PINNED="$JAVA_PIN" FLINK_HOME="$FLINK_PIN" OUT="$WORK/h9_flink" \
            ./run_main_flink.sh || values_ready=0

    if [[ "$values_ready" -eq 0 ]]; then
      undecided "H9 wartosci wobec Flinka — przebiegi nie doszly do konca" "h9-wartosci"
      S_VALUES="NIESPRAWDZONY"
    elif XTRDB="$XTRDB_PIN" python3 run_gates.py --rdb "$WORK/h9_rdb" --flink "$WORK/h9_flink" \
           --plans "$WORK/h9_plans" --work "$WORK/h9_mutants" --out "$WORK/gates.tsv" \
           >"$(log_of "H9 bramki P6")" 2>&1; then
      confirm "H9 wartosci — siedem bramek P6 przeszlo ($WORK/gates.tsv)"
      S_VALUES="ZGODNY"
    elif only_provenance_failed "$WORK/gates.tsv" "$(log_of "H9 bramki P6")"; then
      # Na brudnym drzewie `corpus_validity` NIE MA JAK przejsc i nie jest to
      # dryft. `validate_corpus.py --allow-dirty` stempluje dowod pinem
      # `<sha>-dirty`, a `run_gates.py` porownuje ten pin z czystym SHA i widzi
      # rozjazd. Merytoryczna zawartosc tej bramki — dokladny inwentarz 21 planow,
      # 84/84 kompilacje, 4/4 mutanty, zamkniety manifest — zostala w tym samym
      # przebiegu sprawdzona na poziomie mechanizmu. Bramka dodaje ponad to
      # wylacznie PROWENIENCJE, ktorej brudne drzewo z definicji nie spelnia.
      #
      # Zejscie jest waskie i sprawdzane, nie zalozone: musi zawiesc DOKLADNIE
      # `corpus_validity` i kazda zgloszona rozbieznosc musi byc tym jednym pinem.
      confirm "H9 wartosci — 6/6 bramek merytorycznych przeszlo ($WORK/gates.tsv)"
      S_VALUES="ZGODNY-bez-prow"
      echo "        corpus_validity odpadla WYLACZNIE na pinie SHA (drzewo brudne)."
      echo "        Wartosci sa sprawdzone, proweniencja nie — powtorz na czystym drzewie,"
      echo "        jesli przebieg ma nazywac rewizje."
    else
      refute "H9 wartosci — bramka P6 nie przeszla (log: $(log_of "H9 bramki P6"))"
      S_VALUES="DRYFT"
      echo "        Klasyfikacja porazki bramki nalezy do czlowieka (STOP-6):"
      echo "        engine_or_profile liczy sie PRZECIW H9, apparatus/corpus uniewaznia przebieg."
    fi
  fi

  # ---- poziom czasowy: prog redukcji bajtow substratu i cena czasowa.
  #
  # Jedyny poziom, ktorego ta aparatura NIE sprawdza i sprawdzac nie bedzie.
  # Nie z lenistwa: prog jest wielkoscia ZMIENNA, mierzona na przypietej maszynie
  # brzegowej pod PREEMPT_RT — 1440 komorek, okolo 48 godzin zegara. Nie da sie
  # tego wykonac w celu budowania, a orzekanie o nim z macierzy zmierzonej na
  # INNEJ rewizji silnika byloby zielonym swiatlem dla wlasnosci, ktorej biezacy
  # silnik nigdy nie dotknal — czyli dokladnym przeciwienstwem weryfikacji dryftu.
  undecided "H9 prog czasowy — poza zakresem tej aparatury" "h9-czas"
  S_TIMING="NIESPRAWDZONY"
  echo "        Wielkosc zmienna: 1440 komorek na przypietym pi400 pod PREEMPT_RT, ok. 48 h."
  echo "        Dryft progu czasowego wykrywa kampania pomiarowa, nie to polecenie."
fi

# -------------------------------------------------------------- dziennik
if [[ ! -f "$JOURNAL" ]]; then
  printf 'data\tsilnik\tdrzewo\tziarno\tH10a\tH10b\tH9_mechanizm\tH9_wartosci\tH9_czas\twynik\n' \
    >"$JOURNAL"
fi
# Sam brak dryftu nie jest sprawdzeniem: przebieg, w ktorym nic sie fizycznie
# nie wykonalo, nie moze wygladac na czesciowo udany.
if [[ "$DRIFTED" -eq 1 ]]; then
  OVERALL="DRYFT WYKRYTY"
elif [[ "$CHECKED" -eq 0 ]]; then
  OVERALL="NIC NIE SPRAWDZONO"
elif [[ "${#UNCHECKED[@]}" -eq 0 ]]; then
  OVERALL="BEZ DRYFTU, pelny zakres"
else
  OVERALL="BEZ DRYFTU, zakres niepelny"
fi
printf '%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\n' \
  "$(date -Is)" "$ENGINE_SHA" "$TREE" "$SEED" \
  "$S_H10A" "$S_H10B" "$S_MECH" "$S_VALUES" "$S_TIMING" "$OVERALL" >>"$JOURNAL"

# -------------------------------------------------------------- podsumowanie
echo
echo "=============================================================="
echo " WERYFIKACJA DRYFTU na silniku $ENGINE_SHA (drzewo $TREE, ziarno $SEED)"
printf '   H10a  rachunek poczatku i ogona     : %s\n' "$S_H10A"
printf '   H10b  wystarczalnosc reguly lokalnej: %s\n' "$S_H10B"
printf '   H9    mechanizm wspoldzielenia      : %s\n' "$S_MECH"
printf '   H9    wartosci wobec Flinka         : %s\n' "$S_VALUES"
printf '   H9    prog czasowy i cena           : %s\n' "$S_TIMING"
cat <<'EOF'

 Co znaczy "BEZ DRYFTU", a czego nie znaczy:
   * znaczy, ze silnik NA TYM PRZEBIEGU zgodzil sie z modelem zdarzeniowym i,
     jesli poziom wartosci sie wykonal, z niezalezna implementacja w Flinku;
   * NIE jest powtorzeniem kampanii i nie zastepuje K24d ani K26v3 jako
     dokumentu obowiazujacego dla artykulu;
   * nie siega poza zbadana klase — H10 pyta o korpus generatora (q <= 5,
     dziewiec klas operatorow), H9 o klase Q=8 w trzech rodzinach;
   * nie obejmuje progu czasowego H9, ktory jest wielkoscia mierzona, nie
     obliczana, i wymaga kampanii na maszynie brzegowej.
EOF
echo " Dziennik: $JOURNAL"
if [[ "${#UNCHECKED[@]}" -gt 0 ]]; then
  echo " NIESPRAWDZONE: ${UNCHECKED[*]}"
fi
if [[ "$STRICT" -eq 1 && "${#UNCHECKED[@]}" -gt 0 ]]; then
  DRIFTED=1
  echo " STRICT: poziom niesprawdzony liczy sie jako niezaliczony"
fi
echo " WYNIK: $OVERALL"
echo "=============================================================="
exit "$DRIFTED"
