#!/bin/bash
cd benchmark

TARGET="grpc php-src redis folly micropython ncnn cocos2d-x swoole-src taichi rocksdb tensorflow godot"
INSTALL_PLATFORM="cpython php"

rm -rf install/taichi-venv install/tensorflow-venv install/php-8.3.2_log install/cpython_log

for i in $INSTALL_PLATFORM; do
    ./scripts/install_${i}.sh log
    if [ ! -d "./install/${i}_asan" ]; then
        ./scripts/install_${i}.sh asan
    fi
done

# Build
for p in $TARGET; do
    echo "Build $p with CMASan(log)"
   ./scripts/${p}.sh log
    if [ ! -d "./src/${p}_asan" ]; then
        ./scripts/${p}.sh asan
    fi
done
