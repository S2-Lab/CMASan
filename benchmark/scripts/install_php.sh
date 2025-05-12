#!/bin/bash
export PNAME="php-install"

# Environment settings
source env/env_auto.sh $1
export INSTALL_DIR=`pwd`/install/php-8.3.2_$1
rm -rf $INSTALL_DIR

# Clone
git clone --branch php-8.3.2 https://github.com/php/php-src src/$PNAME
cd src/$PNAME

# Build
make distclean
./buildconf --force && \
./configure --enable-sockets \
 --with-mysql=mysqlnd \
--with-mysqli=mysqlnd \
--with-pdo-mysql=mysqlnd \
--with-dom --with-openssl --with-zlib --enable-mbstring --with-curl --with-libxml --with-mbstring --with-zip --enable-redis --with-pdo  --with-bcmath \
--prefix=$INSTALL_DIR && \
make -j`nproc` && \
make -j`nproc` install