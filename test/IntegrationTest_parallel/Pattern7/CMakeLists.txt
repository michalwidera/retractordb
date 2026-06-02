# Weryfikuje poprawną identyfikację pól gdy kilka strumieni ma te same nazwy
# (issue #17): coreAB, core0, core1 z polami a/b — kompilator musi rozróżniać
# pola po indeksie strumienia. Sprawdza też strumień samoreferencyjny (str1
# korzysta z własnych pól). Wynik kompilatora porównywany z pattern.txt.

get_filename_component(TestID ${CMAKE_CURRENT_SOURCE_DIR} NAME)
set(TestID "pt_${TestID}")
add_test(
  NAME ${TestID}
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND bash -c "set -e ; \
    xretractor query.rql -c > out.txt ; \
    ${CMAKE_COMMAND} -E compare_files --ignore-eol pattern.txt out.txt")
