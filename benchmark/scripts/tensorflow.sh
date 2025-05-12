#!/bin/bash
export PNAME="tensorflow"

# Environment settings
source env/env_auto.sh $1 with_flags
export CMASAN_EXTRACTOR_PATH=`pwd`/CMASan/scripts/extract_pc.py
export RESULT_DIR=`pwd`/result/$PNAME
export TARGET_TEST=`pwd`/data/mnist.py # https://www.tensorflow.org/tutorials/quickstart/beginner
mkdir -p $RESULT_DIR 

export BAZEL_FLAGS="\
$(
for f in ${CFLAGS}; do
  echo "--conlyopt=${f}" "--linkopt=${f}"
done
for f in ${CXXFLAGS}; do
    echo "--cxxopt=${f}" "--linkopt=${f}"
done

for env_flag in "--action_env" "--repo_env" "--test_env"; do
    echo $env_flag=CMA_LIST=$CMA_LIST
    echo $env_flag=RT_CMA_LIST=$RT_CMA_LIST
    echo $env_flag=BAZEL_COMPILER=$CC
    echo $env_flag=CC=$CC
    echo $env_flag=CXX=$CXX
    echo $env_flag=CMASAN_EXTRACTOR_PATH=$CMASAN_EXTRACTOR_PATH
    echo $env_flag=ASAN_OPTIONS=$ASAN_OPTIONS
    echo $env_flag=CMA_LOG_DIR=$RESULT_DIR
done
echo "--strip=never"
echo "--spawn_strategy=local"
echo "--sandbox_debug"
)"
export ASAN_OPTIONS=$ASAN_OPTIONS:detect_odr_violation=0
export CPYTHON_INSTALL_DIR=`pwd`/install/cpython_$1

# Base Cpython environment
if [ ! -d $CPYTHON_INSTALL_DIR ]; then
    echo "$PNAME is python module. Need to run scripts/install_cpython.sh first!"
    exit 0
    # ./scripts/install_cpython.sh $1
fi

rm -rf ./install/$PNAME-venv
$CPYTHON_INSTALL_DIR/bin/python3 -m venv ./install/$PNAME-venv
. ./install/$PNAME-venv/bin/activate
echo $CPYTHON_INSTALL_DIR

pip install pip numpy wheel packaging requests opt_einsum patchelf
pip install keras_preprocessing --no-deps

# Clone
git clone --branch v2.15.0 https://github.com/tensorflow/tensorflow src/$PNAME || exit

# Build
# Remove all -z defs flags which inhibit the address sanitizer 
find src/$PNAME -type f -exec sed -i 's/-Wl,-z.*defs//g' {} +
find src/$PNAME -type f -exec sed -i 's/-z.*defs//g' {} +

cd src/$PNAME

bazel clean --expunge
echo -e "\n\n\n\nY\n\n\n\n" | ./configure
bazel --output_user_root=$(pwd)/.cache --output_base=$(pwd)/.cache/output build $BAZEL_FLAGS //tensorflow/tools/pip_package:build_pip_package
unset BAZEL_FLAGS

mkdir mnt
./bazel-bin/tensorflow/tools/pip_package/build_pip_package ./mnt
pip install ./mnt/tensorflow-2.15.0-cp310-cp310-linux_x86_64.whl

# Post process
python3 $BENCH_HOME/CMASan/scripts/extract_pc_all.py ./ $RT_CMA_LIST
python3 $BENCH_HOME/CMASan/scripts/extract_pc_all.py $CPYTHON_INSTALL_DIR $RT_CMA_LIST
python3 $BENCH_HOME/CMASan/scripts/extract_pc_all.py $VENV_INSTALL_DIR $RT_CMA_LIST

# Test
rm -rf $RESULT_DIR/*
export ASAN_OPTIONS=$ASAN_OPTIONS:halt_on_error=0
export LD_PRELOAD=$(realpath $($CC -print-file-name=libstdc++.so))
export TEST_CMD=". $BENCH_HOME/install/$PNAME-venv/bin/activate && python3 $TARGET_TEST && deactivate"
$BENCH_HOME/scripts/test_generator.sh $1
