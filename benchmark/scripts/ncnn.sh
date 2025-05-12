#!/bin/bash
export PNAME="ncnn"

# Environment settings
source env/env_auto.sh $1
export RESULT_DIR=`pwd`/result/$PNAME
mkdir -p $RESULT_DIR 

# Clone
git clone https://github.com/Tencent/ncnn src/$PNAME
cd src/$PNAME
git checkout a31f66203be958c6df2960fd1601ec98ca313496

# Patch the BOF we found
patch src/layer/x86/shufflechannel_x86.cpp < $BENCH_HOME/data/shufflechannel_x86_bof.patch 

# Build
rm -rf build && mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release -DNCNN_VULKAN=OFF -DNCNN_BUILD_TESTS=ON -DNCNN_BUILD_EXAMPLES=ON ../ && \
    make -j`nproc` && \
    make -j`nproc` benchncnn

# Post process
python3 $LLVM_HOME/../scripts/extract_pc_all.py  ./ $RT_CMA_LIST

# Test
rm -rf $RESULT_DIR/*

cd ../benchmark
export ASAN_OPTIONS=$ASAN_OPTIONS:halt_on_error=0
export CMASAN_QUARANTINE_SIZE_I=15
export TEST_CMD="../build/benchmark/benchncnn"
$BENCH_HOME/scripts/test_generator.sh $1