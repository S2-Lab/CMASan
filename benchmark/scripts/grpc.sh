#!/bin/bash
export PNAME="grpc"

# Environment settings
source env/env_auto.sh $1 "with_flags"
export RESULT_DIR=`pwd`/result/$PNAME
mkdir -p $RESULT_DIR 

export CMASAN_EXTRACTOR_PATH=`pwd`/CMASan/scripts/extract_pc.py

# Clone
git clone --recursive https://github.com/grpc/grpc src/$PNAME
cd src/$PNAME
git checkout b5c5b7a264add29d33e5b374c875da5d80c2e60a
git submodule update --init --recursive

# Build
export BAZEL_FLAGS="\
$(
for f in ${CFLAGS}; do
  echo "--conlyopt=${f}" "--linkopt=${f}"
done
for f in ${CXXFLAGS}; do
    echo "--cxxopt=${f}" "--linkopt=${f}"
done
echo "--copt=-DGRPC_ASAN"
echo "--copt=-DGPR_NO_DIRECT_SYSCALLS"

for env_flag in "--action_env" "--repo_env" "--test_env"; do
    echo $env_flag=CMA_LIST=$CMA_LIST
    echo $env_flag=RT_CMA_LIST=$RT_CMA_LIST
    echo $env_flag=BAZEL_COMPILER=$CC
    echo $env_flag=CC=$CC
    echo $env_flag=CXX=$CXX
    echo $env_flag=CMASAN_EXTRACTOR_PATH=$CMASAN_EXTRACTOR_PATH
    echo $env_flag=ASAN_OPTIONS=$ASAN_OPTIONS
    echo $env_flag=CMA_LOG_DIR=$RESULT_DIR
done
echo "--strip=never"
echo "--spawn_strategy=local"
echo "--sandbox_debug"
)"

export BAZEL_USER_ROOT=$(pwd)/.cache

echo -e "\n" | tools/run_tests/start_port_server.py

bazel --output_user_root=$BAZEL_USER_ROOT clean --expunge
bazel --output_user_root=$BAZEL_USER_ROOT test $BAZEL_FLAGS //test/cpp/... 

export GRPC_PID=`ps -ef | grep tools/run_tests/python_utils/port_server.py | tail -n 1 | awk '{print $2 }'`

if [ -n "$GRPC_PID" ]; then
  echo "Try to kill grpc server(pid: $GRPC_PID)"
  kill -15 $GRPC_PID
fi

# Post process
# python3 $LLVM_HOME/../scripts/extract_pc_all.py  ./ $RT_CMA_LIST >> cache_log
# python3 $LLVM_HOME/../scripts/extract_pc_all.py  ./.cache $RT_CMA_LIST >> cache_log
# python3 $LLVM_HOME/../scripts/extract_pc_all.py  ~/.cache/bazel $RT_CMA_LIST

# Test
export TEST_CMD="$BENCH_HOME/data/grpc_test.sh"
$BENCH_HOME/scripts/test_generator.sh $1
