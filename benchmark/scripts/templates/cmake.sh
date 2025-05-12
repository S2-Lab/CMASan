#!/bin/bash
export PNAME="<your application name>"

# Environment settings
source env/env_auto.sh $1
export RESULT_DIR=`pwd`/result/$PNAME
mkdir -p $RESULT_DIR 

# Clone
git clone <your application repository> src/$PNAME
cd src/$PNAME

# Build
<your application build command Make/CMake>

# Post process
python3 $LLVM_HOME/../scripts/extract_pc_all.py  ./ $RT_CMA_LIST

# Test
rm -rf $RESULT_DIR/*

cd ../benchmark
export TEST_CMD="<your application test command>"
$BENCH_HOME/scripts/test_generator.sh $1