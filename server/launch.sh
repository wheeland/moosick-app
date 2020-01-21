#!/bin/bash

#lighttpd -f lighttpd.conf  -D

lighttpd -f lighttpd.conf  -D &
PID=$!

echo "lighttpd running under PID" $PID

tail -f lighttpd.log

kill $PID
