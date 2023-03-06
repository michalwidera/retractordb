cmake_minimum_required(VERSION 3.3 FATAL_ERROR)

project(database-scripts)

install(PROGRAMS plotblock.py plothistgram.py xdisplay antlr4call.sh buildrdb.sh
        DESTINATION ~/.local/bin)
