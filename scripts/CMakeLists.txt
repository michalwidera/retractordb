cmake_minimum_required(VERSION 3.0 FATAL_ERROR )

project( database-scripts )

set(SCRIPTS_ABRACADABRA
    xplot.sh
    xdump.sh
)

file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/${SCRIPTS_ABRACADABRA}
        DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY} )

