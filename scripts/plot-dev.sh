#!/bin/bash

dd if=/dev/urandom 2>/dev/null |\
hexdump -v -e '/1 "%u 15.5\n"' |\
plotblock.py 100 256 "dane1:blue;dane2:red;dane3:green" --sleep 0.02 |\
gnuplot 2>/dev/null
