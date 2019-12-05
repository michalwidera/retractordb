cmake_minimum_required (VERSION 2.5)
project (db-project)

enable_testing()

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/build)

add_subdirectory(src)
add_subdirectory(test)
add_subdirectory(scripts)

add_custom_target(git-clean
   COMMAND git clean -d -f -x
)

add_custom_target( aptget
    COMMAND sudo apt-get install gcc cmake libboost-all-dev make build-essential cppcheck graphviz gpicview gnuplot tmux feh
    #COMMAND sudo apt-get install tmux 
)

add_custom_target( astyle-build 
    WORKING_DIRECTORY astyle
    COMMAND make
)

add_custom_target( astyle
    WORKING_DIRECTORY src
    COMMAND ../astyle/astyle *.cpp,*.h cpp --style=google --indent=spaces -S -xG -C -K -xU -N -r -j || exit 0
)

add_custom_target( cppcheck
    cppcheck --enable=all -I/usr/local/include/boost src/compiler/* src/share/* src/dumper/* src/abracadabra/* src/qry/*
)

add_custom_target( thesis
    WORKING_DIRECTORY .
    COMMAND test/gopic test/query-thesis.txt
)

add_custom_target( compile
    WORKING_DIRECTORY .
    COMMAND build/xcompiler -q test/query-fast.txt -v 
)
