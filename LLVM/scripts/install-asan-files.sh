#!/bin/bash

#Path to llvm source tree
llvm=`pwd`/llvm
src=`pwd`/src
runtime=`pwd`/compiler-rt
clang=`pwd`/clang

#install LLVM files
llvm_files="lib/Transforms/Instrumentation/AddressSanitizer.cpp include/llvm/Support/CMASanInfoParser.h"
for file in $llvm_files
do
rm $llvm/$file
ln -s $src/llvm-files/`basename $file` $llvm/$file
done
rm  $llvm/utils/gn/secondary/compiler-rt/lib/asan/BUILD.gn
ln -s $src/llvm-files/asan/BUILD.gn $llvm/utils/gn/secondary/compiler-rt/lib/asan/BUILD.gn

runtime_files="lib/asan/asan_interceptors.cpp lib/asan/asan_allocator.cpp lib/asan/asan_rtl.cpp lib/asan/asan_utils.cpp lib/asan/asan_utils.h lib/asan/asan_errors.cpp  lib/asan/asan_errors.h lib/asan/CMakeLists.txt lib/asan/asan_report.cpp lib/asan/asan_report.h lib/asan/asan_mapping.h lib/asan/asan_interface.inc lib/asan/asan_descriptions.cpp"
for file in $runtime_files
do
rm $runtime/$file
ln -s $src/compiler-rt-files/`basename $file` $runtime/$file
done

clang_files="lib/CodeGen/CodeGenModule.h lib/CodeGen/CodeGenModule.cpp"
for file in $clang_files
do
rm $clang/$file
ln -s $src/clang-files/`basename $file` $clang/$file
done


rm $clang/examples/CMakeLists.txt
ln -s $src/clang-files/examples/CMakeLists.txt $clang/examples/CMakeLists.txt

rm -rf $clang/examples/ExtractFunctions
ln -s $src/clang-files/examples/ExtractFunctions $clang/examples/ExtractFunctions

rm -rf $clang/examples/AnnotateCMAFunctions
ln -s $src/clang-files/examples/AnnotateCMAFunctions $clang/examples/AnnotateCMAFunctions