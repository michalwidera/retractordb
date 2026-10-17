# Weryfikuje walidację storage.dir z konfiguracji TOML: - katalog nie istnieje
# -> błąd konfiguracji i przerwanie startu - katalog istnieje, ale brak prawa
# zapisu -> błąd konfiguracji i przerwanie startu

get_filename_component(TestID ${CMAKE_CURRENT_SOURCE_DIR} NAME)
set(TestID "it_${TestID}")

# covers: launcher.xretractor.18~1, launcher.xretractor.40~1, launcher.xretractor.42~1
add_test(
  NAME ${TestID}-nonexistent
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND bash storage-validation.sh nonexistent)

# covers: launcher.xretractor.40~1, launcher.xretractor.42~1
add_test(
  NAME ${TestID}-unwritable
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND bash storage-validation.sh unwritable)

# #430: dwa procesy na wspólnym katalogu wybierają różne nazwy pliku próby
# zapisu
add_test(
  NAME ${TestID}-distinct-probe
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND bash storage-validation.sh distinct-probe)

# covers: launcher.xretractor.39~1
add_test(
  NAME ${TestID}-config-errors
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND python3 config-errors.py $<TARGET_FILE:xretractor>
          $<TARGET_FILE:xqry> $<TARGET_FILE:xtrdb>)
