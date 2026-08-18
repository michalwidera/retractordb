cmake_minimum_required(VERSION 3.20 FATAL_ERROR)

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

# Profile ablacji H9 mieszkaja w build/K26v3-*. Bramka sama sprawdza, czy odcisk
# tresci src/ zapisany przy ich budowie zgadza sie z biezacym — jesli nie zgadza
# sie, profili nie ma albo odcisku brak, poziom 84/84 jest POMINIETY, nigdy
# zaliczony po cichu. Zbudowac: test/research_gate/h9/build_profiles.sh
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
