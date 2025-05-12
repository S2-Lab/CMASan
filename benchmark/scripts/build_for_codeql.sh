#!/bin/bash
export CCACHE_DISABLE=1
export DISABLE_SCCACHE=1
cd /root/benchmark
./scripts/$1.sh codeql

# CodeQL DB build does not work with skbuild & sccache in taichi directly
SKBUILD_DIR=$( find src/$1 -path "*/_skbuild/*/cmake-build" -type d) 
if [ -n $SKBUILD_DIR ]; then
    # Disable sscache by overwriting the PATH with a dummy sccache
    mkdir -p temp/sccache_bin
    echo "#!/bin/bash
    eval $@" > temp/sccache_bin/sccache
    chmod +x temp/sccache_bin/sccache
    export PATH=$(pwd)/temp/sccache_bin:$PATH

    cd $SKBUILD_DIR
    ninja -t clean
    ninja -v
fi