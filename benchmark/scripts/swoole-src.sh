#!/bin/bash
export PNAME="swoole-src"

# Environment settings
source env/env_auto.sh $1 builtin
export RESULT_DIR=`pwd`/result/$PNAME
mkdir -p $RESULT_DIR 

# Install PHP
export PHP_INSTALL_DIR=`pwd`/install/php-8.3.2_$1
if [ ! -d $PHP_INSTALL_DIR ]; then
    echo "$PNAME is php module. Need to run scripts/install_php.sh first!"
    exit 0
    # ./scripts/install_php.sh $1
fi

export PATH=$PHP_INSTALL_DIR/bin:$PATH
export LD_LIBRARY_PATH=$PHP_INSTALL_DIR/lib
export LIBRARY_PATH=$PHP_INSTALL_DIR/include

export CFLAGS="$CFLAGS -DHAVE_MMAP" 
export CXXFLAGS="$CXXFLAGS -DHAVE_MMAP"
export LDFLAGS="$LDFLAGS -DHAVE_MMAP"

# Clone
git clone --branch v5.1.2 https://github.com/swoole/swoole-src src/$PNAME
cd src/$PNAME

php -r "copy('https://getcomposer.org/installer', 'composer-setup.php');"
php composer-setup.php --install-dir=$PHP_INSTALL_DIR/bin --filename=composer

# Build
make distclean
phpize && \
 ./configure --enable-openssl --enable-mysqlnd --enable-swoole-curl --enable-cares --enable-swoole-pgsql && make -j`nproc` && \
make install
echo "extension=swoole.so" > $PHP_INSTALL_DIR/lib/php.ini
php -v
php -m
php --ini
php --ri swoole

# Prepare octane test
git clone https://github.com/laravel/octane.git --depth=1 --branch v2.3.1
cd octane
composer update --prefer-dist --no-interaction --no-progress

pushd vendor/orchestra/testbench-core/laravel && composer install && popd

# Test
rm -rf $RESULT_DIR/*
export TEST_CMD="APP_ENV=testing APP_DEBUG=true OCTANE_SERVER_HOST=0.0.0.0 vendor/bin/phpunit"
$BENCH_HOME/scripts/test_generator.sh $1

