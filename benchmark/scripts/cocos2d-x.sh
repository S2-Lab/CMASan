#!/bin/bash
export PNAME="cocos2d-x"

# Environment settings
source env/env_auto.sh $1
export RESULT_DIR=`pwd`/result/$PNAME
mkdir -p $RESULT_DIR 
# source env/env_cmasan.sh
# source env/env_asan.sh

# Clone
git clone https://github.com/cocos2d/cocos2d-x src/$PNAME
cd src/$PNAME
git checkout 76903dee64046c7bfdba50790be283484b4be271
sed -i 's#git://#https://#' .gitmodules # the submodules can not find public key for ccs-res
echo -e "y\n" | ./install-deps-linux.sh
echo -e "no\n" | python3 download-deps.py
git submodule update --init

# conflict avoidance for Ubuntu 22.04 (https://github.com/cocos2d/cocos2d-x/issues/20471)
wget https://github.com/cocos2d/cocos2d-x/files/4272227/libchipmunk7.0.1.zip
rm -rf external/chipmunk/prebuilt/linux/64-bit
unzip libchipmunk7.0.1.zip -d external/chipmunk/prebuilt/linux/64-bit

# Build

# Disable Native Asan conflicting test cases (SEGV)
export CFLAGS="-DCC_ENABLE_CHIPMUNK_INTEGRATION=0" CXXFLAGS="-DCC_ENABLE_CHIPMUNK_INTEGRATION=0"
sed -i "s#ADD_TEST_CASE(LabelIssue16717);#// ADD_TEST_CASE(LabelIssue16717);#" tests/cpp-tests/Classes/LabelTest/LabelTestNew.cpp
sed -i "s#ADD_TEST_CASE(LabelIssueLineGap);#// ADD_TEST_CASE(LabelIssueLineGap);#" tests/cpp-tests/Classes/LabelTest/LabelTestNew.cpp
sed -i "s#ADD_TEST_CASE(TankExample);#// ADD_TEST_CASE(TankExample);#" tests/cpp-tests/Classes/SpineTest/SpineTest.cpp
sed -i "s#ADD_TEST_CASE(SpineboyExample);#// ADD_TEST_CASE(SpineboyExample);#" tests/cpp-tests/Classes/SpineTest/SpineTest.cpp
sed -i "s#ADD_TEST_CASE(SkeletonRendererSeparatorExample);#// ADD_TEST_CASE(SkeletonRendererSeparatorExample);#" tests/cpp-tests/Classes/SpineTest/SpineTest.cpp
sed -i "s#ADD_TEST_CASE(RaptorExample);#// ADD_TEST_CASE(RaptorExample);#" tests/cpp-tests/Classes/SpineTest/SpineTest.cpp
sed -i "s#ADD_TEST_CASE(MixAndMatchExample);#// ADD_TEST_CASE(MixAndMatchExample);#" tests/cpp-tests/Classes/SpineTest/SpineTest.cpp
sed -i "s#ADD_TEST_CASE(IKExample);#// ADD_TEST_CASE(IKExample);#" tests/cpp-tests/Classes/SpineTest/SpineTest.cpp
sed -i "s#ADD_TEST_CASE(GoblinsExample);#// ADD_TEST_CASE(GoblinsExample);#" tests/cpp-tests/Classes/SpineTest/SpineTest.cpp
sed -i "s#ADD_TEST_CASE(SpriteSlice9Test4);#// ADD_TEST_CASE(SpriteSlice9Test4);#" tests/cpp-tests/Classes/SpriteTest/SpriteTest.cpp
sed -i "s#ADD_TEST_CASE(TestIsFileExistRejectFolder);#// ADD_TEST_CASE(TestIsFileExistRejectFolder);#" tests/cpp-tests/Classes/FileUtilsTest/FileUtilsTest.cpp
sed -i "s#ADD_TEST_CASE(AudioControlTest)#return;ADD_TEST_CASE(AudioControlTest)#" tests/cpp-tests/Classes/NewAudioEngineTest/NewAudioEngineTest.cpp
sed -i "s#addTest(#// addTest(#" tests/cpp-tests/Classes/UITest/UITest.cpp
# Fix BOF(detected by ASan) in rapidXML
sed -i 's/if (\*text == 0 || text >= endptr_)/if (text >= endptr_ || \*text == 0)/g' external/rapidxml/rapidxml_sax3.hpp

# Patch for automatic test start and end
sed -i 's/_testController = TestController::getInstance();/_testController = TestController::getInstance();\n    _testController->startAutoTest();\n/g' tests/cpp-tests/Classes/AppDelegate.cpp
sed -i 's/_sleepUniqueLock = nullptr;/_sleepUniqueLock = nullptr;\n    exit(0);/g' tests/cpp-tests/Classes/controller.cpp

export BUILD_DIR=build
cmake -B $BUILD_DIR -S .
cmake --build $BUILD_DIR
cd $BUILD_DIR

# Post process
python3 $LLVM_HOME/../scripts/extract_pc_all.py  ./ $RT_CMA_LIST

# Test
export ASAN_OPTIONS=$ASAN_OPTIONS:replace_intrin=0:detect_stack_use_after_return=0
rm -rf $RESULT_DIR/*

export BENCH_TEST_USE_GUI=1
export TEST_CMD="./bin/cpp-tests/cpp-tests"

$BENCH_HOME/scripts/test_generator.sh $1
