#!/bin/bash

echo -e "\n" | tools/run_tests/start_port_server.py
echo $BAZEL_FLAGS
bazel --output_user_root=$BAZEL_USER_ROOT test $BAZEL_FLAGS --cache_test_results=no //test/cpp/...
#--sandbox_tmpfs_path=/ 

export GRPC_PID=`ps -ef | grep tools/run_tests/python_utils/port_server.py | tail -n 1 | awk '{print $2 }'`
if [ -n "$GRPC_PID" ]; then
  echo "Try to kill grpc server(pid: $GRPC_PID)"
  kill -15 $GRPC_PID
fi
