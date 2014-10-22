#!/bin/bash

#####################################################################
#
# This file is licensed under the University of Illinois/NCSA Open 
# Source License. See LICENSE.TXT for details.
#
#####################################################################

#####################################################################
#
# Name: test.sh
#
# Description: Compiles main.c running the pass 'Foo' over the 
#              LLVM IR which inturn called FlipIt to corrupt 
#              certain instructions.
#
#####################################################################

echo "
############ COMPILING main.c ############

"
clang -emit-llvm -o main.bc -c main.c
llvm-link $FLIPIT_PATH/src/corrupt/corrupt.bc main.bc  -o crpt.bc
opt -load $LLVM_BUILD_PATH/Debug+Asserts/lib/Foo.so -Foo < crpt.bc > final.bc

clang final.bc -L$FLIPIT_PATH/lib -lcorrupt

echo "
############ RUNNING PROGRAM ############"
./a.out
rm *.bc
