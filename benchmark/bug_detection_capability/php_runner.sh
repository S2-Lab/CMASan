#!/bin/bash

if [[ -z "$CMASAN_HOME" ]]; then
    echo "Please set CMASAN_HOME to the root directory of CMASan"
    exit 1
fi

if [ "$2" = "asan" ]; then
    export CC=$CMASAN_HOME/LLVM-pure/build/bin/clang
    export CXX=$CC++
    export CFLAGS="-fsanitize=address"
    export CXXFLAGS=$CFLAGS LDFLAGS=$LDFLAGS
    SUFFIX="-asan"
else 
    export CC=$CMASAN_HOME/LLVM/bin/cmasan-clang
    export CXX=$CC++
    export CFLAGS="-fsanitize=address"
    export CXXFLAGS=$CFLAGS LDFLAGS=$LDFLAGS
fi

PROG="php"
ISSUE=$1
IDENTIFIER="$PROG-issue-$ISSUE$SUFFIX"
DIR=src/$IDENTIFIER
export CMA_LIST=$(pwd)/cma_list/$IDENTIFIER.json
export RT_CMA_LIST=$(pwd)/temp/$IDENTIFIER.txt

if [ $ISSUE = "11028" ]; then
    COMMIT="915b283"
    COMMAND="./sapi/cli/php ../../corpus/poc-11028.php"
elif [ $ISSUE = "10581" ]; then
    COMMIT="915b283"
    COMMAND="./sapi/cli/php -f ../../corpus/poc-10581.php"
elif [ $ISSUE = "68976" ]; then # CVE-2015-2787
    COMMIT="55da1fb"
    COMMAND="./sapi/cli/php ../../corpus/poc-68976.php"
else
echo "Issue $ISSUE not found"
exit 0
fi

export ASAN_OPTIONS=detect_leaks=0:log_path=$(pwd)/result/${2}-${1}

# Disable shim
export USE_TRACKED_ALLOC=0 USE_ZEND_ALLOC=1

REPO_DIR=src/repo_php-src
if [ ! -d $REPO_DIR ]; then
    git clone https://github.com/php/php-src.git $REPO_DIR
fi

if [ ! -d $DIR ]; then
    cp -r $REPO_DIR $DIR
    cd $DIR
    git checkout $COMMIT
    ./buildconf
    ./configure
    make -j`nproc`
else    
    cd $DIR
fi
sed -i 's/__builtin_constant_p(\([^)]*\))/0/g' Zend/zend_alloc.h
./buildconf
./configure
make -j`nproc`

eval $COMMAND
