# cd build/Debug && make install && ctest -R issue121_isnull -V

get_filename_component(TestID ${CMAKE_CURRENT_SOURCE_DIR} NAME)

# isnull(field) returns 1 when the field is null, 0 when it is not.
# Row 1: a=10, b=NULL  → isnull(a)=0, isnull(b)=1
# Row 2: a=NULL, b=20  → isnull(a)=1, isnull(b)=0
# Row 3: a=30, b=40    → isnull(a)=0, isnull(b)=0
add_test(
  NAME ${TestID}-run
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND
    bash -c "set -e ; \
    mkdir -p temp ; \
    xretractor query.rql -k -x & \
    while [ ! -f /tmp/xretractor_service.lock ]; do sleep 1; done ; \
    xqry -w -s dst -k -m 3 > out.txt ; \
    while [ -f /tmp/xretractor_service.lock ]; do sleep 1; done ; \
    grep -F '0 1' out.txt ; \
    grep -F '1 0' out.txt ; \
    grep -F '0 0' out.txt")
