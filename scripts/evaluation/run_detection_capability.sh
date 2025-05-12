#!/bin/bash
export CMASAN_HOME=$(pwd)
IMAGEMAGICK_IDENTIFIERS="1615 1714 1641 1621 1644 1025 4446"
PHP_IDENTIFIERS="11028 10581 68976"
IMAGEMAGICK_IDENTIFIERS="1615"

rm -rf benchmark/bug_detection_capability/src/* benchmark/bug_detection_capability/result/*
mkdir -p benchmark/bug_detection_capability/result benchmark/bug_detection_capability/temp
cd benchmark/bug_detection_capability

export ASAN_OPTIONS=detect_leaks=0:log_path=$(pwd)/crash/asan.crash

for i in $IMAGEMAGICK_IDENTIFIERS; do
    echo "Run ImageMagick issue $i"
    ./imagemagick_runner.sh $i cmasan
    ./imagemagick_runner.sh $i asan
done

for i in $PHP_IDENTIFIERS; do
    echo "Run PHP issue $i"
    ./php_runner.sh $i cmasan
    ./php_runner.sh $i asan
done