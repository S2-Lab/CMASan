source ./env/common_env.sh $1
log_arg=$1

export CC=$LLVM_HOME/bin/cmasan-clang
export CXX=$CC++