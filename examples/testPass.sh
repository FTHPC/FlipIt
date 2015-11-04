#!/bin/bash
#####################################################################
#
# Simple example that corrupts 2+2. Here we use the long way of 
# compiling.
#
#####################################################################

echo "
#include <stdio.h>
#include \"$FLIPIT_PATH/src/corrupt/corrupt.h\"
int main(int argc, char** argv)
{
	int a = 2, b=2;
	int rank = 0;
	int seed = 7;
	FLIPIT_Init(rank, argc, argv, seed);
 	printf(\"Should be corrupted: %d + %d = %d\n\", a, b, work(a, b));
	FLIPIT_Finalize(NULL);
	return 0;
}" > main.c

echo "int work(int a, int b)
{
	return a + b;
}" > work.c



clang  -g -emit-llvm work.c  -c -o work.bc 
#llvm-link $FLIPIT_PATH/src/corrupt/corrupt.bc work.bc  -o crpt_work.bc
#opt -load $LLVM_BUILD_PATH/Debug+Asserts/lib/FlipIt.so -FlipIt -singleInj 1 -prob 0.95 -byte -1 -arith 1 -ctrl 0 -ptr 0 -funcList "" < crpt_work.bc > final.bc
llvm-link $FLIPIT_PATH/include/FlipIt/corrupt/corrupt.bc work.bc  -o crpt_work.bc
opt -load $FLIPIT_PATH/lib/libFlipItPass.so -FlipIt -srcFile "work.c" -singleInj 1 -prob 0.95 -byte -1 -arith 1 -ctrl 0 -ptr 0 -funcList "" < crpt_work.bc > final.bc
clang  -g -c final.bc -o final.o  

gcc -o main.o -c main.c
gcc -o test final.o main.o -L$FLIPIT_PATH/lib/ -lcorrupt
./test
