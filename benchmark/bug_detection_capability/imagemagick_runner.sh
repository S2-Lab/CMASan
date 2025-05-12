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

PROG="imagemagick"
ISSUE=$1
IDENTIFIER="$PROG-issue-$ISSUE$SUFFIX"
DIR=src/$IDENTIFIER
export CMA_LIST=$(pwd)/cma_list/$IDENTIFIER.json
export RT_CMA_LIST=$(pwd)/temp/$IDENTIFIER.txt

if [ $ISSUE = "1615" ]; then # CVE-2019-13307
    COMMIT="bfc2e47"
    COMMAND='./utilities/magick -seed 0 -monitor -bias 63 "(" magick:rose -colorize 172,35,77  ")" "(" magick:logo +repage ")" -crop 507x10'!'+20-54 -evaluate-sequence Median   tmp'
elif [ $ISSUE = "1714" ]; then
    COMMIT="cfd829b"
    COMMAND='./utilities/magick ../../corpus/PoC_palm.bmp -compress RLE tmp.palm'
elif [ $ISSUE = "1641" ]; then # CVE-2019-17541
    COMMIT="39f226a"
    COMMAND='./utilities/magick ../../corpus/ImageMagick_crash_0 /dev/null'
elif [ $ISSUE = "1621" ]; then
    COMMIT="daac28a"
    COMMAND='./utilities/magick -debug Resource -log "%" "(" magick:wizard "tmp"'
elif [ $ISSUE = "1644" ]; then
    COMMIT="64ede28"
    COMMAND='./utilities/magick convert ../../corpus/poc-1644 /dev/null'
elif [ $ISSUE = "1025" ]; then # CVE-2018-8804
    COMMIT="f55d3a6"
    COMMAND='./utilities/magick convert ../../corpus/not_kitty.jpg not_kitty.EPT2'
elif [ $ISSUE = "4446" ]; then # CVE-2021-3962
    COMMIT="82775af"
    COMMAND="./utilities/magick convert -adjoin -alpha copy -antialias -append -auto-gamma -auto-level  -auto-orient ../../corpus/poc-4446  /dev/null"
else
echo "Issue $ISSUE not found"
exit 0
fi

# Disable Shim
export CFLAGS="$CFLAGS -DMAGICKCORE_ANONYMOUS_MEMORY_SUPPORT" 
export CXXFLAGS=$CFLAGS LDFLAGS=$CFLAGS

export ASAN_OPTIONS=detect_leaks=0:log_path=$(pwd)/result/${2}-${1}

echo "Target: $IDENTIFIER"

REPO_DIR=src/repo_imagemagick
if [ ! -d $REPO_DIR ]; then
    git clone https://github.com/ImageMagick/ImageMagick.git $REPO_DIR
fi

if [ ! -d $DIR ]; then
    cp -r $REPO_DIR $DIR
    cd $DIR
    git checkout $COMMIT
    make clean && make distclean
    ./configure --disable-openmp
    make -j`nproc`
else    
    cd $DIR
fi

echo "Running the command: $COMMAND"
eval $COMMAND
