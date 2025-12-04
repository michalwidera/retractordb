#!/bin/bash

trap control_c SIGINT

control_c()
{
    stty sane
    xqry -k
}

if ! xretractor query-6.rql -c; then exit 1 ; fi

nohup xretractor query-6.rql </dev/null >/dev/null 2>&1 &
echo $! > xretractor.pid
sleep 1
xqry -a 'select * stream stradhoc from core0'
sleep 1
xqry -k
if ps -p $(cat xretractor.pid) > /dev/null 2>&1
then
    kill -9 $(cat xretractor.pid)
fi
rm -f xretractor.pid
while [ -f /tmp/xretractor_service.lock ]; do sleep 1; done ;
