#!/usr/bin/env bash
timeout 10 xretractor brokenQuery.rql -c 2>out.txt
ret=$?
test $ret -ne 0 && grep -q 'Circular dependency' out.txt
