#!/bin/bash
export PNAME="folly"

# Environment settings
source env/env_auto.sh $1
export RESULT_DIR=`pwd`/result/$PNAME
mkdir -p $RESULT_DIR 

# Clone
git clone --branch v2023.12.25.00 https://github.com/facebook/folly src/$PNAME
cd src/$PNAME

sed -i 's/TIMEOUT 120/TIMEOUT 300/g' CMake/FollyFunctions.cmake

# Build
export LDFLAGS="-latomic"
./build/fbcode_builder/getdeps.py install-system-deps --recursive
./build/fbcode_builder/getdeps.py --allow-system-packages build --scratch-path `pwd`/folly_build

# Post process
python3 $LLVM_HOME/../scripts/extract_pc_all.py  ./ $RT_CMA_LIST

# Test
rm -rf $RESULT_DIR/*
cd folly_build/build/folly

# Disable Native Asan conflicting test cases
TEST_FAILED_BY_ASAN="iobuf_test.IOBuf.WrapBuffer|HHWheelTimerTest.HHWheelTimerTest.*|thread_local_test.ThreadLocal.SHAREDLibraryTestName|atomic_unordered_map_test.AtomicUnorderedInsertMap.DISABLEDMegaMap|file_util_test.WriteFileAtomic.directoryPermissions|RequestContextTest.RequestContextTest.ThreadId"
# Expected failures due to CMA instrumentation (redzone/quarantine); excluded to ensure a fair comparison with ASan (otherwise, CMASan terminates earlier than ASan).
TEST_FAILED_BY_CMASAN_LEGITIMATE="|arena_test*|thread_cached_arena_test.ThreadCachedArena.BlockSize"

export TEST_CMD="ctest --timeout 0 -E \"${TEST_FAILED_BY_ASAN}${TEST_FAILED_BY_CMASAN_LEGITIMATE}\""
$BENCH_HOME/scripts/test_generator.sh $1