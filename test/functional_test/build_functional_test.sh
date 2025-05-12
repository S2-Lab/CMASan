#!/bin/bash
export CMA_LIST=functional_test.json
export RT_CMA_LIST=functional_test.txt
export ASAN_OPTIONS=use_sigaltstack=false

rm functional_test
rm functional_test.cmasan.pc
../../LLVM/bin/cmasan-clang++ functional_test.cpp -fsanitize=address $2 -g -o functional_test  # -emit-llvm -S
# clang++  -Xclang -load -Xclang  ~/D-CMA/LLVM/build/lib/AnnotateCMAFunctions.so -Xclang -add-plugin -Xclang annotate-cma-fns -Xclang -plugin-arg-annotate-cma-fns -Xclang sample.json functional_test.cpp -fsanitize=address -o functional_test -g