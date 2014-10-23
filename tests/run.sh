#!/bin/bash

# parameters:
# $1 number of threads
# $2 maximum number of iterations
# $3 synchronization frequency
# $4 the cores to use

LD_PRELOAD=../lib/libreeact.so /usr/bin/time -v ./sync -t $1 -c "$4" -m $2 -n $3 -l "./libsyncworker.so" -f "mul_worker" -v
