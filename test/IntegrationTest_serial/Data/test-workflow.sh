#!/bin/sh

if [ -z "$1" ] || [ -z "$2" ] || [ -z "$3" ]; then
  echo "Usage: $0 <query.rql> <stream1> <stream2>"
  exit 1
fi

pkill xretractor 2>/dev/null
sleep 0.5
rm -f core*
rm -f str*

QUEUES_BEFORE=$(ls /dev/shm/brcdbr* 2>/dev/null | wc -l)

if ! xretractor "$1" -c; then exit 1; fi

xretractor "$1" -m 100 -k -x &
XRETRACTOR_PID=$!

# Poll until xretractor accepts IPC queries (up to 5 seconds)
READY=0
for i in $(seq 1 10); do
  sleep 0.5
  if ! kill -0 $XRETRACTOR_PID 2>/dev/null; then
    echo "xretractor exited unexpectedly"
    exit 1
  fi
  if xqry -d > /dev/null 2>&1; then
    READY=1
    break
  fi
done

if [ $READY -ne 1 ]; then
  echo "xretractor not ready after 5 seconds"
  kill $XRETRACTOR_PID 2>/dev/null
  exit 1
fi

xqry -d

if ! xqry -s "$2" -m 3; then
  kill $XRETRACTOR_PID 2>/dev/null
  exit 1
fi

if ! xqry -s "$3" -m 3; then
  kill $XRETRACTOR_PID 2>/dev/null
  exit 1
fi

xqry -l
xqry -k || true

kill $XRETRACTOR_PID 2>/dev/null
wait $XRETRACTOR_PID 2>/dev/null
pkill xretractor 2>/dev/null; true

QUEUES_AFTER=$(ls /dev/shm/brcdbr* 2>/dev/null | wc -l)
if [ "$QUEUES_AFTER" -gt "$QUEUES_BEFORE" ]; then
  echo "LEAK: brcdbr queue count increased from $QUEUES_BEFORE to $QUEUES_AFTER"
  ls /dev/shm/brcdbr* 2>/dev/null
  exit 1
fi
