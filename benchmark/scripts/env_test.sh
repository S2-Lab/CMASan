#!/bin/bash
export PNAME="env_test"
source ./env/env_auto.sh $1 $2

env | grep \
    -e PNAME= -e CC= -e CXX= -e CMA_LIST= \
    -e LLVM_HOME= -e CMA_LIST= -e RT_CMA_LIST= -e ASAN_OPTIONS= \
    -e CMA_LOG_DIR= -e CFLAGS -e CXXFLAGS -e LDFLAGS

./scripts/clean.sh $PNAME