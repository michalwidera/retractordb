#/bin/sh
#skrypt do testu regresyjnego pod liuxem - puszczane z automatu
echo "-----------------------------------------------------------------"
if  ! xcompiler -t ; then exit 1 ; fi
echo "-----------------------------------------------------------------"
if  ! xretractor -t ; then exit 1 ; fi
echo "-----------------------------------------------------------------"
if ! xcompiler -q test/query-lnx.txt ; then exit 1 ; fi
echo "-----------------------------------------------------------------"
echo "\n SUKCES 1 z 3\n"
echo "-----------------------------------------------------------------"
if ! xdumper -d ; then exit 1 ; fi
echo "-----------------------------------------------------------------"
echo "\n SUKCES 2 z 3\n"
echo "-----------------------------------------------------------------"
if ! xretractor -m 20 --waterfall ; then exit 1 ; fi
echo "-----------------------------------------------------------------"
echo "\n SUKCES 3 z 3\n"
echo "-----------------------------------------------------------------"
#dot -Tjpg out.dot -o out.jpg
#gpicview out.jpg
