# cd build/Debug && make install && ctest -R issue128_string_to_numeric -V

get_filename_component(TestID ${CMAKE_CURRENT_SOURCE_DIR} NAME)
set(TestID "it_${TestID}")

# Weryfikuje konwersję pól STRING do typów numerycznych za pomocą funkcji
# to_integer(), to_float(), to_double(). Dane wejściowe: STRING "10", "25", "-5"
# → to_integer: 10, 25, -5 to_float:   10, 25, -5 to_double:  10, 25, -5
add_test(
  NAME ${TestID}-run
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND
    bash -c "
set -e
mkdir -p temp
xretractor query.rql -k -x &
while [ ! -f /tmp/xretractor_service.lock ]
do sleep 0.1
done
xqry -s dst -k -m 4 > out.txt
while [ -f /tmp/xretractor_service.lock ]
do sleep 0.1
done
grep -F '11 10 10' out.txt
grep -F '26 25 25' out.txt
grep -F -- '-4 -5 -5' out.txt
grep -F 'null null null' out.txt
")

# TODO(lock-path): test zawiesza się lokalnie. Bariera "czekaj na lock"
# hardkoduje /tmp/xretractor_service.lock, ale serwer tworzy lock w
# temp_directory_path() (= $TMPDIR jeśli ustawione, zob. lockManager.cpp:18).
# Gdy TMPDIR != /tmp, pętla "while [ ! -f /tmp/...lock ]" czeka w
# nieskończoność. Naprawa: przenieść logikę do skryptu .sh i użyć
# ${TMPDIR:-/tmp}/xretractor_service.lock (składnia $ nie przechodzi przez makro
# add_test w CMakeLists). Do czasu naprawy: DISABLED.
set_tests_properties(${TestID}-run PROPERTIES DISABLED TRUE)
