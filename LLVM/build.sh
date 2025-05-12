#!/bin/bash
if [ ! -d build ]; then
    #get LLVM
    if [ ! -d llvm ]; then
    wget --retry-connrefused --tries=100 https://github.com/llvm/llvm-project/releases/download/llvmorg-15.0.6/llvm-15.0.6.src.tar.xz
    tar -xf llvm-15.0.6.src.tar.xz
    mv llvm-15.0.6.src llvm
    rm llvm-15.0.6.src.tar.xz
    fi

    #get cmake
    if [ ! -d cmake ]; then
    wget --retry-connrefused --tries=100 https://github.com/llvm/llvm-project/releases/download/llvmorg-15.0.6/cmake-15.0.6.src.tar.xz
    tar -xf cmake-15.0.6.src.tar.xz
    mv cmake-15.0.6.src cmake
    rm cmake-15.0.6.src.tar.xz

    pushd llvm
    ln -s ../cmake .
    popd
    fi

    #get Clang
    if [ ! -d clang ]; then
    wget --retry-connrefused --tries=100 https://github.com/llvm/llvm-project/releases/download/llvmorg-15.0.6/clang-15.0.6.src.tar.xz
    tar -xf clang-15.0.6.src.tar.xz
    mv clang-15.0.6.src clang
    rm clang-15.0.6.src.tar.xz
    pushd llvm/tools
    ln -s ../../clang .
    popd
    fi

    #get compiler-rt
    if [ ! -d compiler-rt ]; then
    wget --retry-connrefused --tries=100 https://github.com/llvm/llvm-project/releases/download/llvmorg-15.0.6/compiler-rt-15.0.6.src.tar.xz
    tar -xf compiler-rt-15.0.6.src.tar.xz
    mv compiler-rt-15.0.6.src compiler-rt
    rm compiler-rt-15.0.6.src.tar.xz
    # pushd llvm/projects
    # ln -s ../../compiler-rt .
    # popd
    fi

    #get libunwind
    if [ ! -d libunwind ]; then
    wget --retry-connrefused --tries=100 https://github.com/llvm/llvm-project/releases/download/llvmorg-15.0.6/libunwind-15.0.6.src.tar.xz
    tar -xf libunwind-15.0.6.src.tar.xz
    mv libunwind-15.0.6.src libunwind
    rm libunwind-15.0.6.src.tar.xz
    fi

    #get lld
    if [ ! -d lld ]; then
    wget --retry-connrefused --tries=100 https://github.com/llvm/llvm-project/releases/download/llvmorg-15.0.6/lld-15.0.6.src.tar.xz

    tar -xf lld-15.0.6.src.tar.xz
    mv lld-15.0.6.src lld
    rm lld-15.0.6.src.tar.xz

    pushd llvm/tools
    ln -s ../../lld .
    popd
    fi

    if [ ! -d binutils ]; then
    git clone --depth 1 git://sourceware.org/git/binutils-gdb.git binutils
    mkdir build
    cd build
    ../binutils/configure --enable-gold --enable-plugins --disable-werror
    make all-gold
    cd ..
    fi

    # ./scripts/install-s2lab-files.sh
    ./scripts/install-asan-files.sh
    make -j`nproc`
fi

#############################
###### CMASan wrappers ######
#############################
LLVM_HOME=`pwd`/build
mkdir -p bin && pushd bin
COMMON_FLAGS="-O2 -Wno-unused-command-line-argument"
### Instrument ###
CMASAN_PASS_ARGS="-mllvm -asan-cma-writeback-path=\$RT_CMA_LIST $COMMON_FLAGS -Xclang -load -Xclang  $LLVM_HOME/lib/AnnotateCMAFunctions.so -Xclang -add-plugin -Xclang annotate-cma-fns -Xclang -plugin-arg-annotate-cma-fns -Xclang \$CMA_LIST -fsanitize=address -fsanitize-recover=address -g -fno-omit-frame-pointer -fno-optimize-sibling-calls"
echo "#!/bin/sh
. $LLVM_HOME/../bin/env
$LLVM_HOME/bin/clang $CMASAN_PASS_ARGS \$@
" > cmasan-clang
echo "#!/bin/sh
. $LLVM_HOME/../bin/env
$LLVM_HOME/bin/clang++ $CMASAN_PASS_ARGS \$@
" > cmasan-clang++

chmod +x ./*

echo "
# CMA_LIST=cma.json
# RT_CMA_LIST=cma.txt
# FNAME_DIR=`pwd`
" > env

popd