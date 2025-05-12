source ./env/common_env.sh pure

export CC=$LLVM_HOME/build/bin/clang
export CXX=$CC++
unset CFLAGS CXXFLAGS LDFLAGS