if ! ./xcompiler -t ; then exit 1 ; fi 
if ! ./xexecutor -t ; then exit 1 ; fi 
if ! ./xcompiler -q regtest/query-lnx.txt ; then exit 1 ; fi 
if ! ./xdumper -d ; then exit 1 ; fi 
if ! ./xexecutor -m 20 ; then exit 1 ; fi 
#dot -Tjpg out.dot -o out.jpg
#gpicview out.jpg
