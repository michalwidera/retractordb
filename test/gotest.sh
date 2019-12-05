#/bin/sh
#skrypt do testu regresyjnego pod liuxem - puszczane z automatu
echo "-----------------------------------------------------------------"
if  ! ./build/xcompiler -t ; then exit 1 ; fi
echo "-----------------------------------------------------------------"
if  ! ./build/xexecutor -t ; then exit 1 ; fi
echo "-----------------------------------------------------------------"
if ! ./build/xcompiler -q test/query-lnx.txt ; then exit 1 ; fi 
echo "-----------------------------------------------------------------"
echo "\n SUKCES 1 z 3\n"
echo "-----------------------------------------------------------------"
if ! ./build/xdumper -d ; then exit 1 ; fi 
echo "-----------------------------------------------------------------"
echo "\n SUKCES 2 z 3\n"
echo "-----------------------------------------------------------------"
if ! ./build/xexecutor -m 20 --waterfall ; then exit 1 ; fi
echo "-----------------------------------------------------------------"
echo "\n SUKCES 3 z 3\n"
echo "-----------------------------------------------------------------"
#dot -Tjpg out.dot -o out.jpg
#gpicview out.jpg
