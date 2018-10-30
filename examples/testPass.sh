#!/bin/bash
#####################################################################
#
# Simple example that corrupts 2+2. Here we use the long way of 
# compiling.
#
#####################################################################

echo "
#include <stdio.h>
#include \"FlipIt/corrupt/corrupt.h\"

int work(int a, int b);

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


# compile work.c
$LLVM_BUILD_PATH/bin/clang  -g -emit-llvm work.c  -c -o work.bc 
$LLVM_BUILD_PATH/bin/llvm-link $FLIPIT_PATH/include/FlipIt/corrupt/corrupt.bc work.bc  -o crpt_work.bc
$LLVM_BUILD_PATH/bin/opt -load $FLIPIT_PATH/lib/libFlipItPass.so -FlipIt -srcFile "work.c" -singleInj 1 -prob 0.95 -byte -1 -bit 0 -arith 1 -ctrl 0 -ptr 0 -funcList "" crpt_work.bc  -o final.bc
$LLVM_BUILD_PATH/bin/clang  -fPIC -g -c final.bc -o final.o  

# display the compiler log file for work.LLVM.txt
$FLIPIT_PATH/scripts/binary2ascii.py work.c.LLVM.bin -o work.c.LLVM.txt
cat work.c.LLVM.txt

# build the executable
gcc -I$FLIPIT_PATH/include -o main.o -c main.c
gcc -o test final.o main.o -L$FLIPIT_PATH/lib/ -lcorrupt
./test
