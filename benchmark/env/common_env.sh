if [ "$1" = "asan" ] || [ "$1" = "pure" ]; then
    llvm_suffix="-pure"
elif [ "$1" = "log" ]; then
    llvm_suffix="-log"
else
    llvm_suffix=""
fi

export BENCH_HOME=`pwd`
export LLVM_HOME=`pwd`/../LLVM$llvm_suffix
export PATH=$LLVM_HOME/build/bin:$PATH

export CMA_LIST=$BENCH_HOME/cma_list/$PNAME.json
export RT_CMA_LIST=$BENCH_HOME/temp/$PNAME.txt
export ASAN_OPTIONS=$ASAN_OPTIONS:log_path=$BENCH_HOME/result/$PNAME/asan.crash:print_cmdline=1:detect_leaks=0:use_sigaltstack=0
echo "" > $RT_CMA_LIST
export CMA_LOG_DIR=$BENCH_HOME/result/$PNAME
export CCACHE_DISABLE=1