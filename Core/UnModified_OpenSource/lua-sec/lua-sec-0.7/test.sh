#!/bin/sh

set -e
set -x

cd samples/oneshot/

lua server.lua &
sleep 1
ret=0
lua client.lua || ret=$?
kill -9 %1 || true
exit $ret
