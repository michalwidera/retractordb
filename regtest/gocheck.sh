#/bin/sh
#skrypt do testu regresyjnego pod liuxem
cppcheck --enable=all -I/usr/local/include/boost console/* compiler/* executor/* share/* dumper/*

