#!/usr/bin/python

"""Time Series Algebra Equations Implementations - Python 3.x
   2025 Michal Widera
"""

from fractions import Fraction
from math import floor, ceil

A = range(1, 24)
deltaA = Fraction(1, 2)
B = list(map(chr, range(ord('a'), ord('z')+1)))
deltaB = Fraction(1, 2)

def sum(A: list, deltaA: Fraction, B: list, deltaB: Fraction):

  result = []
  deltaC = min(deltaA, deltaB)

  for i in range(0, 20):
      if deltaC == deltaA:
          result.append(str(A[i])+B[int(i*deltaA/deltaB)]),
      else:
          result.append(str(A[int(i*deltaB/deltaA)])+B[i]),
  return result, deltaC

def diff(C: list, deltaA: Fraction, deltaB: Fraction):

  result = []
  deltaC = min(deltaA, deltaB)

  for i in range(0, 10):
      if deltaA > deltaB:
          result.append(C[int(ceil(i*deltaA/deltaB))])
      else:
          result.append(C[i])
  return result, deltaC

def hash(A: list, deltaA: Fraction, B: list, deltaB: Fraction):

  result = []
  delta = deltaB / (deltaA + deltaB)

  for i in range(0, 20):
      if floor(i*delta) == floor((i+1)*delta):
          result.append(B[i-int(floor((i+1)*delta))])
      else:
          result.append(A[int(floor(i*delta))])

  deltaC = (deltaA*deltaB)/(deltaA+deltaB)
  return result, deltaC

def dehasheven(C: list, deltaC: Fraction, deltaA: Fraction):

  result = []
  deltaB = deltaA*deltaC / (deltaA - deltaC)

  for i in range(0, 6):
      result.append(C[i+int(ceil((i+1)*deltaA/deltaB))])
  return result, deltaB

def dehashodd(C: list, deltaC: Fraction, deltaB: Fraction):

  result = []
  deltaA = deltaB*deltaC / (deltaB - deltaC)

  for i in range(0, 6):
      result.append(C[i+int(i*deltaB/deltaA)])
  return result, deltaA

def main():
    print("A:", A[0:10], " deltaA:", deltaA)
    print("B:", B[0:10], " deltaB:", deltaB)
    hash_result1, delta_hash1 = hash(A, deltaA, B, deltaB)
    hash_result2, delta_hash2 = hash(B, deltaB, A, deltaA)
    # sum_result, delta_sum = sum(A, deltaA, B, deltaB)
    # diff_result, delta_diff = diff(sum_result, deltaA, deltaB)
    # mod_result, delta_mod = dehasheven(hash_result1, delta_hash1, deltaA)
    # div_result, delta_div = dehashodd(hash_result1, delta_hash1, deltaB)
    # print("Sum:", sum_result[0:10], " deltaSum:", delta_sum)
    print("Hash(A,B):", hash_result1[0:10], " deltaHash:", delta_hash1)
    print("Hash(B,A):", hash_result2[0:10], " deltaHash:", delta_hash2)
    # print("Diff(Sum):", diff_result[0:10], " deltaDiff:", delta_diff)
    # print("Mod(Hash):", mod_result[0:10], " deltaMod:", delta_mod)
    # print("Div(Hash):", div_result[0:10], " deltaDiv:", delta_div)

# A: range(1, 11)  deltaA: 1
# B: ['a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j']  deltaB: 1/2
# Sum: ['1a', '1b', '2c', '2d', '3e', '3f', '4g', '4h', '5i', '5j']  deltaSum: 1/2
# Hash(A,B): ['a', 'b', 1, 'c', 'd', 2, 'e', 'f', 3, 'g']  deltaHash: 1/3
# Hash(B,A): [1, 'a', 'b', 2, 'c', 'd', 3, 'e', 'f', 4]  deltaHash: 1/3
# Diff(Sum): ['1a', '2c', '3e', '4g', '5i', '6k', '7m', '8o', '9q', '10s']  deltaDiff: 1/2
# Mod(Hash): [1, 2, 3, 4, 5, 6]  deltaMod: 1/2
# Div(Hash): ['a', 'b', 'c', 'd', 'e', 'f']  deltaDiv: 1

if __name__ == '__main__':
    main()