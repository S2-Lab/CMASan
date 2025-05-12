#!/bin/bash
export CMA_LOG_DIR=$RESULT_DIR/mem
rm -rf $CMA_LOG_DIR/memory*.log

rm $RESULT_DIR/memory_logger.c $RESULT_DIR/memory_logger.so
cp /root/benchmark/data/memory_logger.c $RESULT_DIR
sed -i "s|<PLACEHOLDER>|$CMA_LOG_DIR|g" $RESULT_DIR/memory_logger.c
sed -i "s|<LOGGER>|$RESULT_DIR/memory_logger.so|g" $RESULT_DIR/memory_logger.c
clang -shared -fPIC -o $RESULT_DIR/memory_logger.so $RESULT_DIR/memory_logger.c 
export LOG_PARSER=$RESULT_DIR/memory_logger.so

mkdir -p $CMA_LOG_DIR
if [[ -z "$BAZEL_FLAGS" ]]; then
	export LD_PRELOAD=$LOG_PARSER:$LD_PRELOAD
	export BAZEL_FLAGS="DUMP"
else
	export BAZEL_FLAGS=`echo $BAZEL_FLAGS|sed "s#test_env=CMA_LOG_DIR=[^[:space:]]*#test_env=CMA_LOG_DIR=$CMA_LOG_DIR#"`
    export BAZEL_FLAGS="$BAZEL_FLAGS --test_env=LD_PRELOAD=$LOG_PARSER"
fi

bash -c "$1"