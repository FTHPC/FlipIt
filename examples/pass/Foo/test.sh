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
$LLVM_BUILD_PATH/bin/clang -g -I$(FLIPIT_PATH)/include -emit-llvm -o main.bc -c main.c
$LLVM_BUILD_PATH/bin/llvm-link $FLIPIT_PATH/src/corrupt/corrupt.bc main.bc  -o crpt.bc
$LLVM_BUILD_PATH/bin/opt -load ./libFooPass.so -Foo crpt.bc -o final.bc
$LLVM_BUILD_PATH/bin/clang final.bc -L$FLIPIT_PATH/lib -lcorrupt

echo "

############ RUNNING PROGRAM ############"
./a.out
rm *.bc
