cmake_minimum_required(VERSION 3.20 FATAL_ERROR)

install(PROGRAMS plotblock.py buildrdb.sh DESTINATION ~/.local/bin)

add_custom_target(
  vimconf
  COMMAND cp -r .vim ~
  WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/scripts)
