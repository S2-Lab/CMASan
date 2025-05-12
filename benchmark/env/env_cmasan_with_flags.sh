source ./env/common_env.sh $1

export CC=$LLVM_HOME/build/bin/clang
export CXX=$CC++
export CFLAGS="-O2 -fno-optimize-sibling-calls -mllvm -asan-cma-writeback-path=$RT_CMA_LIST -Wno-unused-command-line-argument -Xclang -load -Xclang  $LLVM_HOME/build/lib/AnnotateCMAFunctions.so -Xclang -add-plugin -Xclang annotate-cma-fns -Xclang -plugin-arg-annotate-cma-fns -Xclang $CMA_LIST -fsanitize=address -fsanitize-recover=address -g -fno-omit-frame-pointer"
export CXXFLAGS=$CFLAGS LDFLAGS=$CFLAGS