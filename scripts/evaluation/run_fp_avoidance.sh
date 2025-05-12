#!/bin/bash
cd benchmark

export CMASAN_FP_AVOIDANCE_OFF=1

TARGET="grpc php-src redis folly micropython ncnn cocos2d-x swoole-src taichi rocksdb tensorflow godot"

for p in $TARGET; do
    echo "Run $p"
    sleep 15
    ./result/${p}/single_run.sh
    sleep 15
    ./result/${p}_asan/single_run.sh
done