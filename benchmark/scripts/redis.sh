#!/bin/bash
export PNAME="redis"

# Environment settings
source env/env_auto.sh $1
export RESULT_DIR=`pwd`/result/$PNAME
mkdir -p $RESULT_DIR

# Clone
git clone https://github.com/redis/redis src/$PNAME
cd src/$PNAME
git checkout 38f02349462d5aefa9b25386a130ebd67db1a4de

# Build
make BUILD_TLS=yes -j`nproc`

# Post process
python3 $LLVM_HOME/../scripts/extract_pc_all.py  ./ $RT_CMA_LIST

# Test
rm -rf $RESULT_DIR/*
export TEST_CMD="$BENCH_HOME/data/redis_test.sh"

$BENCH_HOME/scripts/test_generator.sh $1
