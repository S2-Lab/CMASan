#!/bin/bash
export PNAME="<your application name>"

# Environment settings
source env/env_auto.sh $1 "with_flags"
export RESULT_DIR=`pwd`/result/$PNAME
mkdir -p $RESULT_DIR 

export CMASAN_EXTRACTOR_PATH=`pwd`/CMASan/scripts/extract_pc.py

# Clone
git clone <your application repository>  src/$PNAME
cd src/$PNAME

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

bazel --output_user_root=$BAZEL_USER_ROOT clean --expunge
bazel --output_user_root=$BAZEL_USER_ROOT build $BAZEL_FLAGS <your application build target>


# Post process
python3 $LLVM_HOME/../scripts/extract_pc_all.py  ./ $RT_CMA_LIST

# Test
export TEST_CMD="<your application test command>"
$BENCH_HOME/scripts/test_generator.sh $1
