# Weryfikuje kompilację i DOT graf zagnieżdżonych podzapytań: (core1#core2)>1
# (hash-merge wewnątrz filtru) oraz (core1>1)#core2 (filtr wewnątrz hash-merge).
# Sprawdza poprawną obsługę kolejności operatorów i zagnieżdżania.

get_filename_component(TestID ${CMAKE_CURRENT_SOURCE_DIR} NAME)
set(TestID "pt_${TestID}")
add_test(
  NAME ${TestID}
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND bash -c "set -e && \
  xretractor query.rql -c > out.txt && \
  ${CMAKE_COMMAND} -E compare_files pattern.txt out.txt")

add_test(
  NAME ${TestID}-dot
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND bash -c "set -e && \
    xretractor query.rql -c -d -f -t -s > out.dot && \
    ${CMAKE_COMMAND} -E compare_files pattern-dot.txt out.dot")
