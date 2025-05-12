#!/bin/bash
pushd functional_test && ./build_functional_test.sh $1 && popd
export RT_CMA_LIST=`pwd`/functional_test/functional_test.txt

ASAN_MODE="asan"
STD_MODE="std"

num2cmd () {
    COMMAND=$1
    ARG=$2

    if [ $COMMAND = "ALLOC" ]; then
        echo 0
    elif [ $COMMAND = "TALLOC" ]; then
        echo 1
    elif [ $COMMAND = "CALLOC" ]; then
        echo 2
    elif [ $COMMAND = "REALLOC" ]; then
        echo 3
    elif [ $COMMAND = "DEALLOC" ]; then
        echo 4
    elif [ $COMMAND = "ACCESS" ]; then
        echo 5
    elif [ $COMMAND = "TACCESS" ]; then
        echo 6
    elif [ $COMMAND = "SIZE" ]; then
        echo 7
    elif [ $COMMAND = "CLEAR" ]; then
        echo 8
    elif [ $COMMAND = "EXIT" ]; then
        echo 9
    elif [ $COMMAND = "INPUT" ]; then
        echo $ARG
    fi
}

file2cmd() {
    FILE=$1
    while IFS= read -r line; do
        num2cmd $line
    done < $FILE
}

check () {
    STD_OUT=`file2cmd functional_test/tests/$1.txt | ./functional_test/functional_test 2>/dev/null`
    ERROR_OUT=`file2cmd functional_test/tests/$1.txt | ./functional_test/functional_test 2>&1 | grep "SUMMARY:" | sed 's/SUMMARY: AddressSanitizer: //g' | sed 's|/.*||g'`

    echo -n "Test $1: "

    if [ $3 = $STD_MODE ]; then
        if [[ $STD_OUT =~ $2 ]] ; then
            echo SUCCESS
        else
            echo FAILED
        fi
    else
        if [ $ERROR_OUT = $2 ]; then
            echo SUCCESS
        else
            echo FAILED
            echo $ERROR_OUT
        fi
    fi
}

check right_bof heap-buffer-overflow-from-CMA $ASAN_MODE
check typed_bof heap-buffer-overflow-from-CMA $ASAN_MODE
check typed_inbound "" $ASAN_MODE
check pool_bof heap-buffer-overflow-from-CMA $ASAN_MODE
check df double-free-from-CMA $ASAN_MODE
check uaf heap-use-after-free-from-CMA $ASAN_MODE
check realloc-shrinked heap-buffer-overflow-from-CMA $ASAN_MODE
check realloc-increased "" $ASAN_MODE
check realloc-df double-free-from-CMA $ASAN_MODE
check calloc_inbound "" $ASAN_MODE
check calloc_bof heap-buffer-overflow-from-CMA $ASAN_MODE
check size_getter "size of chunk 0: 10" $STD_MODE
check clear_uaf heap-use-after-free-from-CMA $ASAN_MODE
check clear_df double-free-from-CMA $ASAN_MODE

# rm cma*.log 2> /dev/null 1> /dev/null
