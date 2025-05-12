FROM ubuntu:22.04

ENV DEBIAN_FRONTEND noninteractive

### Dependencies for base system and CMASan
RUN apt -y update && apt -y upgrade
RUN apt -y install gcc g++ clang vim git binutils cmake wget bison ninja-build build-essential autoconf libtool libtool-bin automake nasm wget flex bison unzip tar apt-utils apt-transport-https sudo
RUN apt-get -y install libedit-dev libgcrypt20-dev libxml2-dev libssl-dev python3-dev libbz2-dev liblzma-dev libdbus-glib-1-dev libgirepository1.0-dev libglib2.0-dev libpixman-1-dev && \ 
    apt-get -y install python3-pip virtualenv python3-setuptools && \
    apt-get -y install linux-tools-common linux-cloud-tools-generic

### Dependencies for benchmark applications
RUN apt install -y mesa-utils libegl1-mesa-dev libgl1-mesa-dev libglu1-mesa-dev xvfb xauth x11-utils libgl1-mesa-glx libglu1-mesa

RUN apt-get install -y ca-certificates curl gnupg python2 tcl8.6 time

RUN apt-get install -y libcurl4-openssl-dev libonig-dev libzip-dev libssl-dev libbz2-dev libgmp-dev libpcre3-dev libcurl4-openssl-dev php-curl libc-ares-dev \
    libpq-dev libgflags-dev libsqlite3-dev re2c freeglut3-dev libglfw3-dev libglm-dev libjpeg-dev liblz4-dev libpng-dev libwayland-dev libx11-xcb-dev \
    libxcb-dri3-dev libxcb-ewmh-dev libxcb-keysyms1-dev libxcb-randr0-dev libxcursor-dev libxi-dev libxinerama-dev libxrandr-dev libzstd-dev libboost-all-dev libsodium-dev \
    libdouble-conversion-dev libevent-dev libgoogle-glog-dev libsnappy-dev libxxf86vm-dev

RUN curl -fsSL https://bazel.build/bazel-release.pub.gpg | gpg --dearmor >bazel-archive-keyring.gpg && \
    mv bazel-archive-keyring.gpg /usr/share/keyrings && \
    echo "deb [arch=amd64 signed-by=/usr/share/keyrings/bazel-archive-keyring.gpg] https://storage.googleapis.com/bazel-apt stable jdk1.8" | sudo tee /etc/apt/sources.list.d/bazel.list && \
    apt -y update && apt install -y bazel && apt install -y bazel-6.1.0

### Locale setting
RUN apt-get update && \
    apt-get install -y locales && \
    echo "en_US.UTF-8 UTF-8" > /etc/locale.gen && \
    locale-gen en_US.UTF-8 && \
    update-locale LANG=en_US.UTF-8 LC_ALL=en_US.UTF-8
ENV LANG en_US.UTF-8
ENV LC_ALL en_US.UTF-8

### Install CodeQL
RUN wget https://github.com/github/codeql-action/releases/download/codeql-bundle-v2.18.2/codeql-bundle-linux64.tar.gz && \
    tar -xvf codeql-bundle-linux64.tar.gz
ENV PATH /codeql:$PATH

RUN pip install tqdm scons rich questionary

### Install CMASan
RUN git clone https://github.com/S2-Lab/CMASan