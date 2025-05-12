#!/bin/bash

# Backup env
declare -p | grep -A 16384 -E "CC|CXX|LD_PRELOAD|BAZEL_FLAGS|LD_LIBRARY_PATH|LIBRARY_PATH|ASAN_OPTIONS|CMA_LOG_DIR|PATH|PNAME|CMASAN_QUARANTINE_SIZE_I|CMASAN_QUARANTINE_ZONE_OFF|RESULT_DIR" > $RESULT_DIR/test_env

# if BENCH_TEST_USE_GUI is set
if [ -n "$BENCH_TEST_USE_GUI" ]; then
    GUI_RUNNER="xvfb-run "
else
    GUI_RUNNER=""
fi

# Generate test script
echo "#!/bin/bash
echo '[CMASan] Setup Test Environment ...'
. $RESULT_DIR/test_env 2> /dev/null
cd $PWD
echo $RESULT_DIR/time.log
echo '[CMASan] Running test ...'
${GUI_RUNNER}/usr/bin/time -v bash -c '$TEST_CMD' &> $RESULT_DIR/time.log
echo '[CMASan] Test done.'" > $RESULT_DIR/single_run.sh
echo "#!/bin/bash
. $RESULT_DIR/test_env 2> /dev/null
cd $PWD
echo '[CMASan] Running test ...'
$GUI_RUNNER$BENCH_HOME/scripts/run_memory.sh '$TEST_CMD' $1
echo '[CMASan] Test done.'" > $RESULT_DIR/memory_run.sh
chmod +x $RESULT_DIR/single_run.sh $RESULT_DIR/memory_run.sh

echo "Run $RESULT_DIR/single_run.sh"
echo "or"
echo "Run $RESULT_DIR/memory_run.sh"