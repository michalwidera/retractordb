if(NOT DEFINED INPUT OR NOT DEFINED OUTPUT)
  message(FATAL_ERROR "INPUT and OUTPUT must be set")
endif()

file(READ "${INPUT}" compile_commands)

# clang-tidy czyta compile_commands.json przez sterownik Clanga. Projekt buduje
# GCC z precompiled headers, a Clang nie przyjmuje GCC-owego PCH ani flagi
# -Winvalid-pch.
string(REGEX REPLACE "[ \t]-Winvalid-pch" "" compile_commands "${compile_commands}")
string(REGEX REPLACE "[ \t]-include[ \t]+[^ \t\"]*cmake_pch\\.hxx" "" compile_commands "${compile_commands}")

get_filename_component(output_dir "${OUTPUT}" DIRECTORY)
file(MAKE_DIRECTORY "${output_dir}")
file(WRITE "${OUTPUT}" "${compile_commands}")
