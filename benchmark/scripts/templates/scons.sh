#!/bin/bash
export PNAME="<your application name>"

# Environment settings
source env/env_auto.sh $1 builtin
export RESULT_DIR=`pwd`/result/$PNAME
mkdir -p $RESULT_DIR 

# Clone
git clone <your application repository> src/$PNAME
cd src/$PNAME

# Build
scons --clean

export USE_ASAN=yes
if [ "$1" == "pure" ]; then
    export USE_ASAN=no
fi
scons platform=linuxbsd use_llvm=yes linker=lld use_asan=$USE_ASAN \
tests=yes dev_build=yes debug_symbols=yes CC=$CC CXX=$CXX -j`nproc`

# Post process
python3 $LLVM_HOME/../scripts/extract_pc_all.py  ./ $RT_CMA_LIST

# Test
rm -rf $RESULT_DIR/*
export TEST_CMD="<your application test command>"

$BENCH_HOME/scripts/test_generator.sh $1
