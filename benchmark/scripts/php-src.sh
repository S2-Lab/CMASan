#!/bin/bash
export PNAME="php-src"

# Environment settings
source env/env_auto.sh $1

export RESULT_DIR=`pwd`/result/$PNAME
mkdir -p $RESULT_DIR

# Clone
git clone https://github.com/php/php-src src/$PNAME || exit
cd src/$PNAME
git checkout 67eb196f1e3402d5dd711708bfbb0db97b1d6d69 # 8.4.0-dev

# Build

make distclean
if [ "$1" != "asan" ] && [ "$1" != "pure" ]; then
    sed -i 's/# define HAVE_BUILTIN_CONSTANT_P//' Zend/zend_portability.h # Disable zend sizeless optimization
fi

./buildconf --force && \
./configure && \
make -j`nproc`

# Post process
python3 $LLVM_HOME/../scripts/extract_pc_all.py  ./ $RT_CMA_LIST

# Test
rm -rf $RESULT_DIR/*

# Expected failures due to CMA instrumentation (redzone/quarantine); excluded to ensure a fair comparison with ASan (otherwise, CMASan terminates earlier than ASan).
export TEST_FAILED_BY_CMASAN_LEGITIMATE="tests/basic/bug80384.phpt Zend/tests/bug43450.phpt Zend/tests/bug79514.phpt Zend/tests/gh8548.phpt Zend/tests/gh8548_2.phpt ext/pcre/tests/bug81243.phpt ext/standard/tests/serialize/bug81142.phpt ext/standard/tests/streams/bug78902.phpt"
rm $TEST_FAILED_BY_CMASAN_LEGITIMATE

export CMASAN_QUARANTINE_SIZE_I=19
export SKIP_ASAN=1 SKIP_PERF_SENSITIVE=1 # PHP ASan configuration (https://github.com/php/php-src/blob/67eb196f1e3402d5dd711708bfbb0db97b1d6d69/run-tests.php#L562)
export TEST_CMD="echo -e 'y\\n' | sapi/cli/php run-tests.php --keep-php -v"
$BENCH_HOME/scripts/test_generator.sh $1
