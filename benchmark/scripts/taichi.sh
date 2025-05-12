#!/bin/bash
export PNAME="taichi"

# Environment settings
source env/env_auto.sh $1 builtin

export RESULT_DIR=`pwd`/result/$PNAME
mkdir -p $RESULT_DIR 
export CPYTHON_INSTALL_DIR=`pwd`/install/cpython_$1
export VENV_INSTALL_DIR=`pwd`/install/$PNAME-venv

# Base Cpython environment
if [ ! -d $CPYTHON_INSTALL_DIR ]; then
    echo "$PNAME is python module. Need to run scripts/install_cpython.sh first!"
    exit 0
    # ./scripts/install_cpython.sh $1
fi

rm -rf ~/.cache/ti-build-cache/

rm -rf $VENV_INSTALL_DIR
$CPYTHON_INSTALL_DIR/bin/python3 -m venv $VENV_INSTALL_DIR
. $VENV_INSTALL_DIR/bin/activate

# Clone
git clone --recursive https://github.com/taichi-dev/taichi src/$PNAME
cd src/$PNAME
git checkout 5a9946127ade6444d2f088f00bfbf7f3428ec418

git fetch origin master --tags --force
git submodule update --init --recursive

# Fix broken vulkan dependency
sed -i 's|https://sdk.lunarg.com/sdk/download/1.3.236.0/linux/vulkansdk-linux-x86_64-1.3.236.0.tar.gz|https://sdk.lunarg.com/sdk/download/1.3.296.0/linux/vulkansdk-linux-x86_64-1.3.296.0.tar.xz|g' .github/workflows/scripts/ti_build/vulkan.py
sed -i 's|1.3.236|1.3.296|g' .github/workflows/scripts/ti_build/vulkan.py
sed -i '/elif name.endswith(".tar.gz") or name.endswith(".tgz"):/,/\ \ \ \ \ \ \ \ tar("-xzf", local_cached, "-C", outdir, f"--strip-components={strip}")/c \
    elif name.endswith(".tar.gz") or name.endswith(".tgz"):\
        outdir.mkdir(parents=True, exist_ok=True)\
        tar("-xzf", local_cached, "-C", outdir, f"--strip-components={strip}")\
    elif name.endswith(".tar.xz"):\
        outdir.mkdir(parents=True, exist_ok=True)\
        tar("-xJf", local_cached, "-C", outdir, f"--strip-components={strip}")' .github/workflows/scripts/ti_build/dep.py

# Build
sed -i 's/sys\.setdlopenflags(2 | 8)/sys.setdlopenflags(2)/g' ./python/taichi/_lib/utils.py # Asan does not support RTLD_BIND flag
export DEBUG=1 

export TAICHI_CMAKE_ARGS="-DCMAKE_POLICY_VERSION_MINIMUM=3.5 -DTI_WITH_VULKAN:BOOL=ON -DCMAKE_C_COMPILER=$CC -DCMAKE_CXX_COMPILER=$CXX -DTI_BUILD_TESTS=ON -DTI_BUILD_EXAMPLES=ON -DTI_BUILD_RHI_EXAMPLES=ON"

pip install -r requirements_test.txt 

sed -i 's/ensurepip = _try_import("ensurepip")/ensurepip = False # _try_import("ensurepip")/' .github/workflows/scripts/ti_build/bootstrap.py

echo -e "y\n" | ./build.py --python=native # build a wheel
pip install dist/* --force-reinstall

python3 $BENCH_HOME/CMASan/scripts/extract_pc_all.py ./ $RT_CMA_LIST
python3 $BENCH_HOME/CMASan/scripts/extract_pc_all.py $CPYTHON_INSTALL_DIR $RT_CMA_LIST
python3 $BENCH_HOME/CMASan/scripts/extract_pc_all.py $VENV_INSTALL_DIR $RT_CMA_LIST

# Test
rm -rf $RESULT_DIR/*
export ASAN_OPTIONS=$ASAN_OPTIONS:protect_shadow_gap=0:replace_intrin=0
cd tests 
export TEST_CMD=". $VENV_INSTALL_DIR/bin/activate && python3 run_tests.py --cpp && deactivate"
$BENCH_HOME/scripts/test_generator.sh $1
