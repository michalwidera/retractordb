cmake_minimum_required(VERSION 3.20...4.4 FATAL_ERROR)

# Bramka badawcza H9/H10.
#
# CELOWO poza `ninja` (brak ALL) i poza `ninja test` (brak add_test): praca przy
# zrodlach silnika nie ma sie o nia zatrzymywac. Uruchamia sie jawnie — `ninja
# test_gate` — z CI okresowego albo recznie przed zgloszeniem artykulu.
#
# Skrypt dziala na katalogu ZRODLOWYM, a wszystko zapisuje do katalogu buildu.
# Nie korzysta z kopii `test/` robionej przez test/CMakeLists.txt, wiec edycja
# aparatury nie wymaga ponownego `cmake .`.

set(RESEARCH_GATE_COUNT
    10010
    CACHE STRING "Liczba planow w kampanii H10 (10010 = pelna kampania K24d)")

# Praca lokalna dopuszcza pominiety poziom (np. brak profili ablacji) i tylko go
# odnotowuje. CI konfiguruje sie z ON: tam pominiecie jest bledem, bo zielona
# bramka o niepelnym zakresie jest gorsza niz jej brak.
option(RESEARCH_GATE_STRICT "Pominiety poziom bramki oblewa przebieg" OFF)
set(RESEARCH_GATE_STRICT_ARG "")
if(RESEARCH_GATE_STRICT)
  set(RESEARCH_GATE_STRICT_ARG "--strict")
endif()

# Profile ablacji H9 mieszkaja w build/K26v3-*. Bramka sprawdza, czy odcisk
# tresci src/ zapisany przy ich budowie zgadza sie z biezacym. Rozjazd odcisku
# automatycznie przebudowuje profile przed poziomem 84/84; brak profilu albo
# odcisku nadal jest POMINIETY, nigdy zaliczony po cichu.
add_custom_target(
  test_gate
  COMMAND
    ${CMAKE_CURRENT_SOURCE_DIR}/run_gate.sh --xretractor
    $<TARGET_FILE:xretractor> --work ${CMAKE_CURRENT_BINARY_DIR}/gate-work
    --count ${RESEARCH_GATE_COUNT} --profiles ${CMAKE_SOURCE_DIR}/build
    ${RESEARCH_GATE_STRICT_ARG}
  DEPENDS xretractor
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  COMMENT "Bramka badawcza H9/H10 (mechanizm, nie potwierdzenie hipotez)"
  USES_TERMINAL VERBATIM)

# Fizyczna weryfikacja braku dryftu — inne pytanie niz `test_gate`. Bramka
# porownuje ETYKIETY rezimow z zamrozona tablica odniesienia na dwoch
# zamrozonych ziarnach. Ten cel WYKONUJE silnik: na swiezo wylosowanej probie
# planow i wobec niezaleznej implementacji (Apache Flink), czyli lapie dryft
# wartosci i dryft poza korpusem tych dwoch ziaren — jedno i drugie jest dla
# bramki niewidoczne z konstrukcji.
#
# Tak samo poza `all` i poza `ninja test`, i z tego samego powodu co bramka —
# tyle ze mocniejszego: przebieg z poziomem wartosci trwa kilkadziesiat minut.
add_custom_target(
  test_drift
  COMMAND
    ${CMAKE_CURRENT_SOURCE_DIR}/run_drift.sh --xretractor
    $<TARGET_FILE:xretractor> --work ${CMAKE_CURRENT_BINARY_DIR}/drift-work
    --count ${RESEARCH_GATE_COUNT} --profiles ${CMAKE_SOURCE_DIR}/build
  DEPENDS xretractor
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  COMMENT "Fizyczna weryfikacja braku dryftu silnika wzgledem wynikow H9/H10"
  USES_TERMINAL VERBATIM)
