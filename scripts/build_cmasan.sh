#!/bin/bash
echo "Build CMASan"

# Set absolute path
sed -i "s#cmasanExtractorPath = \".*/extract_pc.py\"#cmasanExtractorPath = \"`pwd`/scripts/extract_pc.py\"#g" LLVM/src/compiler-rt-files/asan_utils.cpp

# Environment variable
echo "export PATH=`pwd`/LLVM/build/bin:\$PATH" >> ~/.bashrc

pushd LLVM
./build.sh
popd
