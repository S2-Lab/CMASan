#!/bin/bash

# Run the server as background job
{ 
    CMA_LOG_DIR=$CMA_LOG_DIR ./src/redis-server --daemonize no --save "" --enable-protected-configs yes --enable-debug-command yes --enable-module-command yes; 
} &> $RESULT_DIR/result_server.log2 &

echo "Waiting for redis server..."
sleep 10

./runtest --host 127.0.0.1 --port 6379 --tags -slow 

# --skipunit unit/other

REDIS_PID=`ps -ef | grep redis-server | grep -v grep | head -n 1 | awk '{ print $2 }'`
if [ -n "$REDIS_PID" ]; then
  echo "Try to kill redis server(pid: $REDIS_PID)"
  kill -9 $REDIS_PID
fi
