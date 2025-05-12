source ./env/common_env.sh $1

BIN_DIR=`pwd`/temp/${PNAME}_bin
mkdir -p $BIN_DIR
cp $LLVM_HOME/bin/* $BIN_DIR
sed -i "s#\$CMA_LIST#$CMA_LIST#g" $BIN_DIR/*
sed -i "s#\$RT_CMA_LIST#$RT_CMA_LIST#g" $BIN_DIR/*
export CC=$BIN_DIR/cmasan-clang CXX=$BIN_DIR/cmasan-clang++

