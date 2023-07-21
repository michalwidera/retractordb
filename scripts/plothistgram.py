#! /usr/bin/env python3

"""Module that goes probably to legacy."""
import sys
import argparse
import collections

from time import sleep

parser = argparse.ArgumentParser()
parser.add_argument("ymin", default=0, help="ymin dimension in points",type=int)
parser.add_argument("ymax", default=256, help="ymax dimension in points",type=int)
parser.add_argument("--term", default="qt", help="termianl plot type i.e. x11,wxt,qt")
parser.add_argument("--sleep", default=0 , help="put sleep in pipe transfer to plot", type=float)
parser.add_argument("desc", help="data descriptor")
args = parser.parse_args()
prdata = args.desc.split(';')

if (args.term) : print ("set term {} noraise".format(args.term))
print ("set yrange [0:{}]".format(str(args.y)))
print ("set style fill solid")
print ("set ticslevel 0")
print ("set hidden3d")

print ('color(name) = (name eq "gray") ? 0xdddddd \
: (name eq "white") ? 0xffffff \
: (name eq "black") ? 0x000000 \
: (name eq "red")  ?  0xff0000 \
: (name eq "blue") ?  0x0000ff \
: (name eq "green") ? 0x00ff00 \
: (name eq "yellow") ? 0xffff00 \
: int(rand(0)*0xffffff)')

print("boxwidth = 0.5")

try:
    for line in iter(sys.stdin.readline, ''):

        print("plot '-' using 1:3:(boxwidth):(color(strcol(4))):xtic(2) with boxes fillcolor rgb variable notitle")

        record = line.split()

        lineCount = 0
        for i in record:
            print("{} {} {} {}".format(lineCount + 1, prdata[lineCount].split(':')[0] , record[lineCount] , prdata[lineCount].split(':')[1])
            lineCount +=1

        #gnuplot sometimes get too much data and raises memory exception
        if ( args.sleep != 0 ) : sleep( args.sleep )

        print("e")
        sys.stdout.flush()


#ctrl+c to gracefull exit loop
except KeyboardInterrupt:
    pass
