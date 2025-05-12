#!/bin/bash

mkdir benchmark/src benchmark/temp benchmark/result

echo "Build CMASan"

# Set absolute path
sed -i "s#cmasanExtractorPath = \".*/extract_pc.py\"#cmasanExtractorPath = \"`pwd`/scripts/extract_pc.py\"#g" LLVM/src/compiler-rt-files/asan_utils.cpp

# Environment variable
echo "export PATH=`pwd`/LLVM/build/bin:\$PATH" >> ~/.bashrc

pushd LLVM
./build.sh
popd

cp -r LLVM LLVM-log
cp -r LLVM LLVM-pure

echo "Build CMASan(with log)"
pushd LLVM-log
rm -rf build
rm -rf src
ln -s ../LLVM/src src
export CFLAGS="-DCMASAN_LOG_CHECK -DEXPERIMENT_COVERAGE_INCREMENT"
export CXXFLAGS=$CFLAGS LDFLAGS=$CFLAGS
./build.sh
popd

echo "CMASan and CMASan(with log) Build complete!"

echo "Build ASan(pure)"
pushd LLVM-pure
rm -rf build
rm -rf clang compiler-rt llvm
echo "" > ./scripts/install-asan-files.sh
./build.sh
rm -rf bin
popd

echo "Build all completed!"