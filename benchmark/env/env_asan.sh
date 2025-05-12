source ./env/common_env.sh asan

export CC=$LLVM_HOME/build/bin/clang
export CXX=$CC++
# Wrap asan
export BIN_DIR=`pwd`/temp/${PNAME}_bin
mkdir -p $BIN_DIR
ASAN_ARGS="-O2 -fsanitize=address -g -fno-omit-frame-pointer -fno-optimize-sibling-calls"
echo "#!/bin/sh
$CC $ASAN_ARGS \$@" > $BIN_DIR/asan-clang
echo "#!/bin/sh
$CXX $ASAN_ARGS \$@" > $BIN_DIR/asan-clang++
export CC=$BIN_DIR/asan-clang CXX=$BIN_DIR/asan-clang++
chmod +x $BIN_DIR/*
unset BIN_DIR
