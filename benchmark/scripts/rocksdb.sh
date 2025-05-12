#!/bin/bash
export PNAME="rocksdb"

# Environment settings
source env/env_auto.sh $1 with_flags

export RESULT_DIR=`pwd`/result/$PNAME
mkdir -p $RESULT_DIR

# Clone
git clone https://github.com/facebook/rocksdb src/$PNAME
cd src/$PNAME
git checkout 1de69409805e196dae4daad4975f6f83080f8a7c

export CFLAGS="$CFLAGS"
export CXXFLAGS=$CFLAGS LDFLAGS="$CFLAGS -latomic"

# Post process
python3 $LLVM_HOME/../scripts/extract_pc_all.py ./ $RT_CMA_LIST

# Build
rm -rf build && mkdir build && cd build
cmake ../ -DROCKSDB_BUILD_SHARED=OFF
make -j`nproc`

# Test
rm -rf $RESULT_DIR/*
# Expected failures due to CMA instrumentation (redzone/quarantine); excluded to ensure a fair comparison with ASan (otherwise, CMASan terminates earlier than ASan).
export TEST_FAILED_BY_CMASAN_LEGITIMATE="ArenaTest.ApproximateMemoryUsage.*|ArenaTest.Simple.*"
# RocksDB optimizes the database based on the total allocation size as we discussed in Section 6.
export TESTS_FAILED_BY_LIMITATION=".*ColumnFamilyTest.LogDeletionTest.*|.*ColumnFamilyTest.DifferentWriteBufferSizes.*|.*ColumnFamilyTest.FlushAndDropRaceCondition.*|DBCompactionTestWithParam/DBCompactionTestWithParam.FixFileIngestionCompactionDeadlock.*|DBTest.FlushSchedule.*|DBTest.DynamicMemtableOptions.*|DBTest.DynamicCompactionOptions.*|DBTestWithParam/DBTestWithParam.FIFOCompactionTest.*|NumLevels/DBTestUniversalCompaction.ConcurrentBottomPriLowPriCompactions.*|Parallel/DBTestUniversalCompactionParallel.PickByFileNumberBug.*|c_test"
export TEST_CMD="ctest -E \"$TESTS_FAILED_BY_LIMITATION|$TEST_FAILED_BY_CMASAN_LEGITIMATE\""

$BENCH_HOME/scripts/test_generator.sh $1