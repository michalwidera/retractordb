#/bin/sh
#skrypt do testu regresyjnego pod liuxem - do testowania w srodowisku
echo "-----------------------------------------------------------------"
if ! ./xcompiler -q regtest/query.txt ; then exit ; fi
sleep 1
echo "-----------------------------------------------------------------"
nohup ./xexecutor &
read -p "any key to kill"
echo "-----------------------------------------------------------------"
./xqdb -k 
echo "-----------------------------------------------------------------"
