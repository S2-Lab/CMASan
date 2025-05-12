#!/bin/bash
export PNAME="cpython-install"

# Environment settings
source env/env_auto.sh $1 with_flags

export INSTALL_DIR=`pwd`/install/cpython_$1

# Clone
git clone --branch v3.10.9 https://github.com/python/cpython src/$PNAME
cd src/$PNAME

# Build
make clobber
./configure --prefix=$INSTALL_DIR --with-ensurepip=install && \
    make -j`nproc` && \
    make install

python3 $BENCH_HOME/CMASan/scripts/extract_pc_all.py $INSTALL_DIR $RT_CMA_LIST # Generate nothing but PIE offset