cmake_minimum_required(VERSION 3.3 FATAL_ERROR )

project( database-scripts )

install(PROGRAMS plotblock.py plothistgram.py xdisplay DESTINATION /usr/local/bin)

add_custom_target(vim COMMAND mkdir -p ~/.vim && cp -r vim/* ~/.vim )
