#/bin/sh
#skrypt do testu regresyjnego pod liuxem
cppcheck --enable=all -I/usr/local/include/boost ../src/compiler/* ../src/executor/* ../src/share/* ../src/dumper/* 

