cmake_minimum_required(VERSION 3.0 FATAL_ERROR )

project( database-scripts )

install(PROGRAMS plotblock.py plothistgram.py xdisplay DESTINATION /usr/local/bin)
