#!/bin/bash
export PNAME="<your application name>"

# Environment settings
source env/env_auto.sh $1 <refer other templates to fill here>

export RESULT_DIR=`pwd`/result/$PNAME
mkdir -p $RESULT_DIR 
export CPYTHON_INSTALL_DIR=`pwd`/install/cpython_$1
export VENV_INSTALL_DIR=`pwd`/install/$PNAME-venv

# Base Cpython environment
if [ ! -d $CPYTHON_INSTALL_DIR ]; then
    echo "$PNAME is python module. Need to run scripts/install_cpython.sh first!"
    exit 0
fi

rm -rf $VENV_INSTALL_DIR
$CPYTHON_INSTALL_DIR/bin/python3 -m venv $VENV_INSTALL_DIR
. $VENV_INSTALL_DIR/bin/activate

# Clone
git clone <your library repository src/$PNAME
cd src/$PNAME

<your library build command>
pip install <your library wheel>


python3 $BENCH_HOME/CMASan/scripts/extract_pc_all.py ./ $RT_CMA_LIST
python3 $BENCH_HOME/CMASan/scripts/extract_pc_all.py $CPYTHON_INSTALL_DIR $RT_CMA_LIST
python3 $BENCH_HOME/CMASan/scripts/extract_pc_all.py $VENV_INSTALL_DIR $RT_CMA_LIST

# Test
rm -rf $RESULT_DIR/*
export TEST_CMD=". $VENV_INSTALL_DIR/bin/activate && <your library test command>"
$BENCH_HOME/scripts/test_generator.sh $1
