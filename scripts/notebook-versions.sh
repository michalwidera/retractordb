#!/usr/bin/env bash
# Wspolna referencja lokalnych srodowisk i notebook-smoke; nie opisuje
# zmiennego runtime Colaba. Zmiany wersji przechodza oba przebiegi.
readonly RDB_NOTEBOOK_PYTHON=3.12
readonly RDB_NOTEBOOK_NUMPY=2.0.2
readonly RDB_NOTEBOOK_TORCH=2.11.0
readonly RDB_NOTEBOOK_MAC_TORCH=2.14.1
readonly RDB_NOTEBOOK_CUDA=12.8
readonly RDB_NOTEBOOK_CPU_INDEX=https://download.pytorch.org/whl/cpu
readonly RDB_NOTEBOOK_CUDA_INDEX=https://download.pytorch.org/whl/cu128
readonly RDB_NOTEBOOK_LAB=4.6.4
readonly RDB_NOTEBOOK_PACKAGES=(
  "numpy==$RDB_NOTEBOOK_NUMPY"
  "nanobind==3.1.0"
  "pytest==9.1.1"
  "nbclient==0.11.0"
  "nbformat==5.11.1"
  "ipykernel==7.4.0"
)
