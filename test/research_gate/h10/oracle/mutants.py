#!/usr/bin/env python3
"""Zamrożony zestaw mutantów rachunku silnika (predeklarowany).

Mutant psuje replikę w jednym miejscu. Bramka wymaga wykrycia 100% z nich:
mutant jest **wykryty**, gdy w korpusie bramkowym istnieje węzeł, na którym
oracle zgadza się z repliką i nie zgadza się z mutantem. Bez tego warunku
bramka byłaby spełniona przez sam fakt, że oracle różni się od czegokolwiek.

Zestaw dzieli się na dwie rodziny, bo silnik niesie teraz dwie wielkości:

* ``TAIL_MUTANTS`` — cztery mutacje ogona z §10/K24, zachowane bez zmian, plus
  dwie nowe celujące w miejsca, które zmieniło przestemplowanie z 2026-08-06:
  ogon `>N`, który przestał zawierać `N`, i ogon `@`, który przestał zawierać
  człon fazowy. Obie odtwarzają dokładnie stan sprzed zmiany, więc bramka
  odpowiada też na pytanie „czy oracle w ogóle widzi różnicę między starą
  a nową semantyką” — gdyby nie widział, kampania byłaby ślepa;
* ``ORIGIN_MUTANTS`` — mutacje początku logicznego. Wielkość jest nowa i nie
  miała dotąd żadnej bramki; bez nich origin mógłby być cicho błędny, bo suma
  origin+ogon w wielu planach jest niewrażliwa na przesunięcie między członami.
"""

TAIL_MUTANTS = {
    "hash_phase_plus_one": {"hash_phase": 1},
    "hash_phase_minus_one": {"hash_phase": -1},
    "hash_swap_pq": {"hash_swap": True},
    "hash_drop_own": {"hash_drop_own": True},
    "theta_zero_own": {"theta_zero_own": True},
    # --- dolozone w K24/H10 faza 3 (2026-08-18): postacie zastapione naprawa
    # ogona `-`, `Theta` i `~Theta`. Kazda z nich byla do tego dnia rachunkiem
    # SILNIKA, wiec — tak jak `shift_tail_keeps_source` i `hash_closed_form_o1`
    # — jest najwazniejszym rodzajem mutanta: powrot do niej musi byc widoczny.
    #
    # `-` na silniku `0f273d5`: czlon fazowy doklejony do ogona skladowej
    # zamiast do indeksu, plus osobna galaz dla deklaracji (zawyzala o slot
    # w 80,9% wezlow `-` korpusu).
    "subtract_declaration_slot": {"subtract_declaration_slot": True},
    # `Theta` na silniku `0f273d5`: staly czlon wlasny rowny jeden, doklejany do
    # przeliczonego ogona skladowej (zawyzal o slot w 40,3% wezlow `Theta`).
    "theta_constant_own": {"theta_constant_own": True},
    # `~Theta` na silniku `0f273d5`: samo przeliczenie ogona skladowej przez
    # takt, z zaokragleniem w gore liczonym OSOBNO (zawyzalo w 0,8% wezlow).
    "ntheta_rounds_source_tail": {"ntheta_rounds_source_tail": True},
    # Stan sprzed przestemplowania: `N` z powrotem w ogonie przesunięcia.
    "shift_tail_keeps_n": {"shift_tail_keeps_n": True},
    # Stan sprzed przestemplowania: człon fazowy z powrotem w ogonie okna.
    "agse_tail_keeps_phase": {"agse_tail_keeps_phase": True},
    # --- dołożone w K24d: postacie zastąpione naprawami z 2026-08-07 ---------
    #
    # Obie były do 2026-08-07 rachunkiem SILNIKA, nie wymysłem — dlatego są
    # najważniejszymi mutantami zestawu. Gdyby oracle ich nie odróżniał, K24d
    # nie miałaby mocy rozstrzygania, czy naprawy cokolwiek zmieniły.
    #
    # `>N` na silniku `db4a360`: ogon równy ogonowi producenta, wymuszony
    # adresowaniem offsetem względnym w fetchBack (zawyżał o min(W_src, N)).
    "shift_tail_keeps_source": {"shift_tail_keeps_source": True},
    # `#` na silniku `db4a360`: postać O(1) z członem fazowym ceil((p+q-1)/p)
    # doklejonym do przeliczonego ogona drugiej składowej (zawyżała o slot
    # w 7,9% węzłów korpusu).
    "hash_closed_form_o1": {"hash_o1": True},
    # Przegląd okresu fazowego skrócony o połowę — mutant celujący w sposób,
    # w jaki nowa reguła `#` może się zepsuć przy refaktoryzacji: trafia
    # w większość węzłów, ale nie we wszystkie.
    "hash_scan_half_period": {"hash_scan_half_period": True},
}

ORIGIN_MUTANTS = {
    # Okno bez rozpiętości — origin zapomina, że rekord n sięga wstecz o |L|-1.
    "agse_drop_span": {"agse_drop_span": True},
    "agse_origin_plus_one": {"agse_origin_delta": 1},
    "agse_origin_minus_one": {"agse_origin_delta": -1},
    # Przesunięcie bez origin — cały efekt `>N` znika z planu.
    "shift_drop_origin": {"shift_drop_origin": True},
    # Przeplot patrzy tylko na lewą składową; prawa może wtedy być czytana
    # przed swoim początkiem.
    "hash_origin_left_only": {"hash_origin_left_only": True},
}

# Zachowana pod starą nazwą, żeby skrypty odwołujące się do MUTANTS działały
# bez zmian; bramka mutantów rozdziela obie rodziny sama.
MUTANTS = dict(TAIL_MUTANTS)
