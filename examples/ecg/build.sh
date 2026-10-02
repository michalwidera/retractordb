#!/bin/bash
set -o errexit

script_dir="$(cd "$(dirname "$0")" && pwd)"
rec_dir="$script_dir/rec205"

python3 "$script_dir/mitbih2rdb.py" "$rec_dir/205.hea"

# rec205.desc - format identyczny z tym co generuje xretractor po DECLARE ... BINFILE 'rec205'.
# Odwzorowuje dokładnie operator<< klasy Descriptor (descriptor.cc):
# '{' + TAB + pola, bez newline po '{', z REF i TYPE BINFILE na końcu.
printf '{' >  "$rec_dir/rec205.desc"
printf '\tINTEGER MLII\n' >> "$rec_dir/rec205.desc"
printf '\tINTEGER V1\n'   >> "$rec_dir/rec205.desc"
printf '\tREF "rec205"\n' >> "$rec_dir/rec205.desc"
printf '\tTYPE BINFILE\n' >> "$rec_dir/rec205.desc"
printf '}'                >> "$rec_dir/rec205.desc"
