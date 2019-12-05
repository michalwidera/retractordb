#!/bin/bash

me="$(basename "$(test -L "$0" && readlink "$0" || echo "$0")")"

if [ "$1" == "-h" ] ; then
    echo "Usage: $me queryFileName.Ext phaseLnkExt PicType"
    echo "Example: $me query.txt qry png"
    echo "When no parameters given - defaults are used."
    exit
fi

if [ "$1" != "" ] ; then
FILE=$1
else
FILE=query.txt
fi

if [ "$2" != "" ] ; then
PHASE=$2
else
PHASE=qry
fi

if [ "$3" != "" ] ; then
PICTYPE=$3
else
PICTYPE=png
fi

echo 'FILE:' $FILE
echo 'PHASE:' $PHASE
echo 'DUMP:' 'query.'$PHASE

if ! ./build/xcompiler -q $FILE ; then exit 1 ; fi 

echo "Show fields? [Y/n]"
read answer
if [ "$answer" != "${answer#[Yy]}" ] ;then
vFIELDS=-f
fi

echo "Show stream progs ? [Y/n]"
read answer
if [ "$answer" != "${answer#[Yy]}" ] ;then
vSTRPROGS=-s
fi

echo "Show tags ? [Y/n]"
read answer
if [ "$answer" != "${answer#[Yy]}" ] ;then
vTAGS=-t
fi

echo "Show types ? [Y/n]"
read answer
if [ "$answer" != "${answer#[Yy]}" ] ;then
vTYPES=-w
fi

if ! ./build/xdumper -i 'query.'$PHASE -d $vFIELDS $vSTRPROGS $vTAGS $vTYPES > ${FILE%%.*}'.dot' ; then exit 1 ; fi 

dot -T$PICTYPE ${FILE%%.*}'.dot' -o ${FILE%%.*}'.'$PICTYPE

rm ${FILE%%.*}'.dot'

#sudo apt install feh
feh ${FILE%%.*}'.'$PICTYPE

