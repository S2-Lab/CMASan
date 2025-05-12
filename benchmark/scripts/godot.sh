#!/bin/bash
export PNAME="godot"

# Environment settings
source env/env_auto.sh $1 builtin
export RESULT_DIR=`pwd`/result/$PNAME
mkdir -p $RESULT_DIR 

# Clone
git clone https://github.com/godotengine/godot src/$PNAME
cd src/$PNAME
git checkout 89cc635c0554cb2e518c830969ca4c5eedda0f4e

# Build
scons --clean

export USE_ASAN=yes
export GODOT_OUT_BIN="./bin/godot.linuxbsd.editor.dev.x86_64.llvm.san"
if [ "$1" == "pure" ]; then
    export USE_ASAN=no
    export GODOT_OUT_BIN="./bin/godot.linuxbsd.editor.dev.x86_64.llvm"
fi
scons platform=linuxbsd use_llvm=yes linker=lld use_asan=$USE_ASAN \
tests=yes dev_build=yes debug_symbols=yes CC=$CC CXX=$CXX -j`nproc`

# Post process
python3 $LLVM_HOME/../scripts/extract_pc_all.py  ./ $RT_CMA_LIST

# Test
export ASAN_OPTIONS=$ASAN_OPTIONS:halt_on_error=0
git clone https://github.com/godotengine/godot-benchmarks
cd godot-benchmarks && git checkout 950b6548536d8a7b7112bc991ecbd74b75837c40 && cd ../

# If timeout is insufficient, test will not be start.
echo "[$PNAME benchmark]: Close the window if the materials are fully loaded"
timeout 360 xvfb-run -n 11 $GODOT_OUT_BIN --display-driver x11 --rendering-driver opengl3 --path $(pwd)/godot-benchmarks --editor 

rm -rf $RESULT_DIR/*
export BENCH_TEST_USE_GUI=1
export TEST_CMD="$GODOT_OUT_BIN --display-driver x11 --rendering-driver opengl3 --path `pwd`/godot-benchmarks -- --run_benchmarks --save-json=$RESULT_DIR/bench_result.json"

$BENCH_HOME/scripts/test_generator.sh $1
