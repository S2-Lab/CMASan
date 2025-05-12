#!/bin/bash
cd benchmark

TARGET="grpc php-src redis folly micropython ncnn cocos2d-x swoole-src taichi rocksdb tensorflow godot"

for p in $TARGET; do
    echo "Run $p"
    sleep 15
    ./result/${p}/memory_run.sh
    sleep 15
    ./result/${p}_asan/memory_run.sh
done