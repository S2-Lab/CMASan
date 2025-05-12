#!/bin/bash
export PNAME="micropython"

# Environment settings
source env/env_auto.sh $1
export RESULT_DIR=`pwd`/result/$PNAME
mkdir -p $RESULT_DIR

# Clone
rm -rf src/$PNAME
git clone --branch v1.22.0 https://github.com/micropython/micropython src/$PNAME
cd src/$PNAME

# Build
export DEBUG=1

sed -i '/make \${MAKEOPTS} -C mpy-cross/ s/$/ CC=\/usr\/bin\/gcc/' tools/ci.sh # mpy-cross/Makefile # mpy is a build tool for micropython
export FLAGS="-mllvm -asan-stack=0"

mkdir gcc_bin
echo "#!/bin/sh
$CC $FLAGS \$@" > gcc_bin/gcc
echo "#!/bin/sh
$CXX $FLAGS \$@" > gcc_bin/g++
chmod +x gcc_bin/gcc gcc_bin/g++
export PATH=`pwd`/gcc_bin:$PATH

source tools/ci.sh
ci_unix_standard_build

# Post process
python3 $LLVM_HOME/../scripts/extract_pc_all.py  ./ $RT_CMA_LIST

# Test
rm -rf $RESULT_DIR/*
cd tests

export CMASAN_QUARANTINE_SIZE_I=15 CMASAN_QUARANTINE_SIZE_N=64
export MICROPY_MICROPYTHON=../ports/unix/build-standard/micropython
export TEST_CMD="ulimit -n 2048 && python3 run-tests.py"
$BENCH_HOME/scripts/test_generator.sh $1

