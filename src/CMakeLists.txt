cmake_minimum_required(VERSION 3.20 FATAL_ERROR)

# Packages are managed in ./CMakeLists.txt find_package(Boost REQUIRED)
# find_package(spdlog REQUIRED) find_package(fmt REQUIRED)

add_subdirectory(common)
add_subdirectory(rdb)
add_subdirectory(qry)
add_subdirectory(retractor)
if(BUILD_SUPERVISOR)
  add_subdirectory(${CMAKE_SOURCE_DIR}/docker/supervisor
                   ${CMAKE_CURRENT_BINARY_DIR}/supervisor)
endif()

add_custom_target(
  simple
  COMMAND ${CMAKE_SOURCE_DIR}/scripts/xplot.sh
  WORKING_DIRECTORY
    ${CMAKE_CURRENT_BINARY_DIR}/../test/IntegrationTest_serial/simple)

add_custom_target(
  simple-max
  COMMAND ${CMAKE_SOURCE_DIR}/scripts/xplot.sh str2
  WORKING_DIRECTORY
    ${CMAKE_CURRENT_BINARY_DIR}/../test/IntegrationTest_serial/simple_max)

add_custom_target(
  simple-timeshift
  COMMAND ${CMAKE_SOURCE_DIR}/scripts/xplot.sh str1 query-51.rql
  WORKING_DIRECTORY
    ${CMAKE_CURRENT_BINARY_DIR}/../test/IntegrationTest_serial/issue56_timeshift
)

add_custom_target(
  dsp
  COMMAND ${CMAKE_SOURCE_DIR}/scripts/xplot.sh outputAll query.rql 50:256
  WORKING_DIRECTORY
    ${CMAKE_CURRENT_BINARY_DIR}/../test/IntegrationTest_parallel/dsp)

add_custom_target(
  ecg-clean
  COMMAND
    sh -c "rm -f ecg.desc *.shadow STREAM_ADD* bpf.desc df.desc \
      s205out s205out.desc s205out.meta \
      mlii mlii.desc mlii.meta \
      v1 v1.desc v1.meta \
      mlii_win mlii_win.desc mlii_win.meta \
      bp_acc bp_acc.desc bp_acc.meta \
      bp_out bp_out.desc bp_out.meta \
      bp_win bp_win.desc bp_win.meta \
      d_acc d_acc.desc d_acc.meta \
      d_out d_out.desc d_out.meta \
      sq_out sq_out.desc sq_out.meta \
      mwi_win mwi_win.desc mwi_win.meta \
      mwi mwi.desc mwi.meta \
      mwi_long mwi_long.desc mwi_long.meta \
      mwi_thr mwi_thr.desc mwi_thr.meta \
      mwi_proc mwi_proc.desc mwi_proc.meta \
      qrs_out qrs_out.desc qrs_out.meta \
      detect_out detect_out.desc detect_out.meta"
  COMMAND rm -rf temp
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/examples/ecg/rec205)

add_custom_target(
  ecg
  COMMAND ${CMAKE_SOURCE_DIR}/scripts/xplot.sh s205out rec205-replay.rql
    720,560,1360 --gnuplot-rtl
  DEPENDS ecg-clean
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/examples/ecg/rec205)

add_custom_target(
  ecg-qrs
  COMMAND ${CMAKE_SOURCE_DIR}/scripts/xplot.sh qrs_out rec205-qrs.rql
    1440,-250,350 --gnuplot-rtl
  DEPENDS ecg-clean
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/examples/ecg/rec205)

add_custom_target(
  ecg-detect-qrs
  COMMAND ${CMAKE_SOURCE_DIR}/scripts/xplot.sh detect_out rec205-detect.rql
    720,-400,400 --gnuplot-rtl
  DEPENDS ecg-clean
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/examples/ecg/rec205)

add_custom_target(
  simple-view
  COMMAND xretractor query.rql -c -d -f -t -s > out.dot && dot -Tpng out.dot -o
          out.png && feh out.png
  WORKING_DIRECTORY
    ${CMAKE_CURRENT_BINARY_DIR}/../test/IntegrationTest_serial/simple)

add_custom_target(
  show-ninja-build
  COMMAND ninja -t graph | dot -Tpng -o ninja-build.png && feh ninja-build.png
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})

add_custom_target(
  simple-max-view
  COMMAND xretractor query.rql -c -d -f -t -s > out.dot && dot -Tpng out.dot -o
          out.png && feh out.png
  WORKING_DIRECTORY
    ${CMAKE_CURRENT_BINARY_DIR}/../test/IntegrationTest_serial/simple_max)

add_custom_target(
  dsp-view
  COMMAND xretractor query.rql -c -d -f -t -s > out.dot && dot -Tpng out.dot -o
          out.png && feh out.png
  WORKING_DIRECTORY
    ${CMAKE_CURRENT_BINARY_DIR}/../test/IntegrationTest_parallel/dsp)

# python -m pip install clang-format cmakelang
# https://zed0.co.uk/clang-format-configurator/ | $clang-format --version
# clang-format version 21.1.7

add_custom_target(
  cformat
  COMMAND
    find ${CMAKE_SOURCE_DIR}/src -iname "*.h" -o -iname "*.cpp" -o -iname
    "*.hpp" -o -iname "*.cc" | grep --invert-match '.antlr/' | xargs
    clang-format -i
  COMMAND find ${CMAKE_SOURCE_DIR}/test -iname "*.h" -o -iname "*.cpp" -o -iname
          "*.hpp" -o -iname "*.cc" | xargs clang-format -i
  COMMAND find ${CMAKE_SOURCE_DIR} -name CMakeLists.txt | xargs cmake-format -i
          --line-width=80
  WORKING_DIRECTORY ${PROJECT_SOURCE_DIR})

find_program(PROG_CLANG_TIDY NAMES clang-tidy clang-tidy-19 clang-tidy-18
                                   clang-tidy-17 clang-tidy-16 clang-tidy-15)

if(PROG_CLANG_TIDY)
  add_custom_target(
    ctidy
    COMMAND
      find ${CMAKE_SOURCE_DIR}/src -iname "*.cpp" -o -iname "*.cc" | grep
      --invert-match '.antlr/' | xargs -r ${PROG_CLANG_TIDY}
      -p=${CMAKE_BINARY_DIR}
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR})
else()
  message(STATUS "Optional tool not found: clang-tidy (ctidy target disabled)")
endif()

add_custom_target(
  showelf
  COMMAND readelf -p .comment retractor/xretractor
  COMMAND readelf -p .comment rdb/xtrdb
  COMMAND readelf -p .comment qry/xqry
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})

add_custom_target(
  vim
  COMMAND [ ! -f ~/.vim ] && mkdir -p ~/.vim
  COMMAND cp -r ${CMAKE_SOURCE_DIR}/scripts/vim/* ~/.vim)

add_custom_target(
  packages
  COMMAND cpack --config CPackConfig.cmake
  COMMAND cpack --config CPackSourceConfig.cmake
  COMMAND ls -lart *.tar.gz
  WORKING_DIRECTORY ${CMAKE_BINARY_DIR})

# This target is required when VSCode will call nina and CMakeCache.txt is in
# incompatible format - conan build .. complains : CMake Error: Error: generator
# : Unix Makefiles Does not match the generator used previously: Ninja just use:
# $make clear_cache
add_custom_target(clear-cache COMMAND rm ${CMAKE_BINARY_DIR}/CMakeCache.txt)
add_custom_target(patch COMMAND git diff --patch > ${CMAKE_BINARY_DIR}/my.patch)
add_custom_target(
  cppcheck
  COMMAND cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=ON ..
  COMMAND
    cppcheck --inline-suppr --error-exitcode=1
    --project=${CMAKE_BINARY_DIR}/compile_commands.json
    -i${CMAKE_SOURCE_DIR}/test --suppress=*:*.conan*)

find_package(Git QUIET)

string(TIMESTAMP GIT_CURRENT_BUILDTIME "%y%m%d%H%M")
if(GIT_FOUND AND EXISTS "${CMAKE_SOURCE_DIR}/.git")
  execute_process(
    COMMAND ${GIT_EXECUTABLE} branch --show-current
    OUTPUT_VARIABLE GIT_CURRENT_BRANCH
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
  string(STRIP "${GIT_CURRENT_BRANCH}" GIT_CURRENT_BRANCH)
  execute_process(
    COMMAND ${GIT_EXECUTABLE} rev-parse --short HEAD
    OUTPUT_VARIABLE GIT_PREV_COMMIT
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
  string(STRIP "${GIT_PREV_COMMIT}" GIT_PREV_COMMIT)
else()
  set(GIT_CURRENT_BRANCH Deatached)
  set(GIT_PREV_COMMIT none)
endif()
configure_file(config.h.in ${PROJECT_SOURCE_DIR}/src/include/config.h @ONLY)
