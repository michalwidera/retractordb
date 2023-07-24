#! /usr/bin/env python3

"""Module that prepares data for plot program."""
import sys
import argparse
import collections

from time import sleep

# from signal import signal, SIGPIPE, SIG_DFL
# signal(SIGPIPE, SIG_DFL)

parser = argparse.ArgumentParser()
parser.add_argument("x", default=10, help="x dimension in points", type=int)
parser.add_argument("y", default=256, help="y dimension in points", type=int)
parser.add_argument("--term", default="qt", help="terminal plot type i.e. x11,wxt,qt")
parser.add_argument(
    "--sleep", default=0, help="put sleep in pipe transfer to plot", type=float
)
parser.add_argument(
    "--feedinit", default=0, help="feeds gnuplot with initial zeros", type=int
)
parser.add_argument("desc", help="data descriptor")
args = parser.parse_args()

prdata = args.desc.split(";")

collectionList = []
for i in prdata:
    collectionList.append(collections.deque(maxlen=args.x))
i = None

if args.term:
    print(f"set term {args.term} noraise")
print("set style fill transparent solid 0.5")
print(f"set xrange [0:{str(args.x)}]")
print(f"set yrange [0:{str(args.y)}]")
print("set ticslevel 0")
print("set hidden3d")

try:
    for line in iter(sys.stdin.readline, ""):

        print("plot", end="")
        for i in prdata:
            desc = i.split(":")
            print(f" '-' u 1:2 t '{desc[0]}' w lines lc rgb '{desc[1]}'", end="")
            if i != prdata[-1]:
                print(",", end="")
        print()
        i = None

        for i in range(len(prdata)):
            record = line.split()

            if len(record) > i:
                collectionList[i].appendleft(record[i])

                lineCount = 0
                for i in collectionList[i]:
                    print(f"{lineCount} {i}")
                    lineCount += 1

                # gnuplot sometimes get too much data and raises memory exception
                if args.sleep != 0:
                    sleep(args.sleep)

            print("e")
            sys.stdout.flush()
        i = None

# ctrl+c to graceful exit loop
except KeyboardInterrupt:
    pass
