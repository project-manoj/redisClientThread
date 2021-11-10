#!/bin/sh
# add unix sock location
sockAdd=/var/run/redis/redis-server.sock
echo "Flushing all data ... "
redis-cli -s $sockAdd FLUSHALL
echo "Flush done"

echo "Add 50000 data values "
cat data.txt | redis-cli -s $sockAdd --pipe

echo "Data updation done !"

