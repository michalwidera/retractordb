if(NOT DEFINED INPUT OR NOT DEFINED OUTPUT)
  message(FATAL_ERROR "INPUT and OUTPUT must be set")
endif()

file(READ "${INPUT}" compile_commands)

# clang-tidy czyta compile_commands.json przez sterownik Clanga. Projekt buduje
# GCC ze skanowaniem zaleznosci modulow, ale Clang nie rozumie jego flag. Opisuja
# one tylko wykrywanie zaleznosci podczas budowania i nie zmieniaja analizowanej
# jednostki translacji.
string(REGEX REPLACE "[ \t]-fmodules-ts" "" compile_commands "${compile_commands}")
string(REGEX REPLACE "[ \t]-fmodule-mapper=[^ \t\"]+" "" compile_commands "${compile_commands}")
string(REGEX REPLACE "[ \t]-fdeps-format=[^ \t\"]+" "" compile_commands "${compile_commands}")
string(REGEX REPLACE "[ \t]-Winvalid-pch" "" compile_commands "${compile_commands}")
string(REGEX REPLACE "[ \t]-include[ \t]+[^ \t\"]*cmake_pch\\.hxx" "" compile_commands "${compile_commands}")

get_filename_component(output_dir "${OUTPUT}" DIRECTORY)
file(MAKE_DIRECTORY "${output_dir}")
file(WRITE "${OUTPUT}" "${compile_commands}")
