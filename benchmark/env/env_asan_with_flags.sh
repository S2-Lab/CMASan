source ./env/common_env.sh asan
export CC=$LLVM_HOME/build/bin/clang
export CXX=$CC++
export CFLAGS="-O2 -g -fno-omit-frame-pointer -fno-optimize-sibling-calls -fsanitize=address"
export CXXFLAGS=$CFLAGS 
export LDFLAGS=$CFLAGS
