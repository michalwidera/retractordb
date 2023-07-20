cmake_minimum_required(VERSION 3.5 FATAL_ERROR)

# Packages are managed in ./CMakeLists.txt find_package(Boost REQUIRED)
# find_package(spdlog REQUIRED) find_package(fmt REQUIRED)

add_subdirectory(rdb)
add_subdirectory(qry)
add_subdirectory(retractor)

add_custom_target(
  simple
  COMMAND ${CMAKE_SOURCE_DIR}/scripts/plot-simple.sh
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/../test/Regression/Data/simple)

add_custom_target(
  dsp
  COMMAND ${CMAKE_SOURCE_DIR}/scripts/plot-dsp.sh
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/../test/Regression/Data/dsp)

add_custom_target(
  simple-view
  COMMAND xretractor query-simple.rql -c -d -f -t -s > out.dot && dot -Tpng
          out.dot -o out.png && feh out.png
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/../test/Regression/Data/simple)

add_custom_target(
  dsp-view
  COMMAND xretractor query.rql -c -d -f -t -s > out.dot && dot -Tpng out.dot -o
          out.png && feh out.png
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/../test/Regression/Data/dsp)

# sudo apt install clang-format ; sudo pip install cmakelang
# https://zed0.co.uk/clang-format-configurator/
add_custom_target(
  cformat
  COMMAND
    find ${CMAKE_SOURCE_DIR}/src -iname "*.h" -o -iname "*.cpp" -o -iname
    "*.hpp" -o -iname "*.cc" | grep --invert-match 'Parser/' | xargs
    clang-format -i --style='{BasedOnStyle: Google, ColumnLimit: 129}'
  COMMAND
    find ${CMAKE_SOURCE_DIR}/test -iname "*.h" -o -iname "*.cpp" -o -iname
    "*.hpp" -o -iname "*.cc" | xargs clang-format -i --style='{BasedOnStyle:
    Google, ColumnLimit: 129}'
  COMMAND find ${CMAKE_SOURCE_DIR} -name CMakeLists.txt | xargs cmake-format -i
          --line-width=80
  WORKING_DIRECTORY ${PROJECT_SOURCE_DIR})

add_custom_target(
  vim
  COMMAND [ ! -f ~/.vim ] && mkdir -p ~/.vim
  COMMAND cp -r ${CMAKE_SOURCE_DIR}/scripts/vim/* ~/.vim)

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
    cppcheck --inline-suppr --project=${CMAKE_BINARY_DIR}/compile_commands.json
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
