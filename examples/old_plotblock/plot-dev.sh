#!/bin/bash

dd if=/dev/urandom 2>/dev/null |\
hexdump -v -e '1/1 "%u " 1/1 "%u\n"' |\
plotblock.py 100 256 "random_1:blue;random_2:red" --sleep 0.05 |\
gnuplot 2>/dev/null
