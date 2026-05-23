get_filename_component(TestID ${CMAKE_CURRENT_SOURCE_DIR} NAME)
set(TestID "pt_${TestID}")

add_test(
  NAME ${TestID}
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND
    bash -c "set -e ; \
  rm -f plain_file* ret_file* _tmp* out-*.txt ; \
  ${CMAKE_BINARY_DIR}/src/rdb/xtrdb noprompt < term.script > out-gen.txt ; \
  ${CMAKE_BINARY_DIR}/src/rdb/xtrdb -s plain_file > out-plain.txt ; \
  ${CMAKE_BINARY_DIR}/src/rdb/xtrdb -s ret_file > out-ret.txt ; \
  grep -Fq '[====]' out-plain.txt ; \
  grep -Fq '[----]' out-plain.txt ; \
  grep -Fq '[~~~~]' out-plain.txt ; \
  grep -Fq '[XXXX]' out-plain.txt ; \
  grep -Fq 'Storage map: ret_file' out-ret.txt ; \
  grep -Fq 'DATA TOTAL' out-ret.txt ; \
  grep -Fq 's0' out-ret.txt ; \
  grep -Fq 's1' out-ret.txt ; \
  grep -Fq '[====]' out-ret.txt ; \
  grep -Fq '[----]' out-ret.txt ; \
  grep -Fq '[~~~~]' out-ret.txt ; \
  grep -Fq '[XXXX]' out-ret.txt ; \
  grep -Fq 'ROTATED FILES' out-plain.txt ; \
  grep -Fq '[1]' out-plain.txt ; \
  grep -Fq '[0]' out-plain.txt ; \
  grep -Fq 'plain_file.old1' out-plain.txt ; \
  grep -Fq 'plain_file.old0' out-plain.txt ; \
  grep -Fq 'plain_file.meta.old0' out-plain.txt ; \
  grep -Fq 'plain_file.shadow.old0' out-plain.txt ; \
  grep -Fq 'ROTATED FILES' out-ret.txt ; \
  grep -Fq 'ret_file.meta.old2' out-ret.txt")
