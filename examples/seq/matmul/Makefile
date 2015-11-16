#####################################################################
#
# Example that shows how to use the "flip-cc" compiler script. 
# In addition, it shows how to turn the injector on/off.
#
#####################################################################

CC=gcc
CFLAGS = -g -I$(FLIPIT_PATH)/include

FILIB= -L$(FLIPIT_PATH)/lib -lcorrupt
FIPASS= $(FLIPIT_PATH)/lib/libFlipItPass.so
LFLAGS = $(FILIB)

long: matmul.o main.o
	$(CC) -o long matmul.o main.o $(LFLAGS)

matmul.o: matmul.c
	$(LLVM_BUILD_PATH)/bin/clang $(CFLAGS) -emit-llvm matmul.c  -c -o matmul.bc 
	$(LLVM_BUILD_PATH)/bin/llvm-link $(FLIPIT_PATH)/include/FlipIt/corrupt/corrupt.bc matmul.bc  -o crpt_matmul.bc
	$(LLVM_BUILD_PATH)/bin/opt -load $(FIPASS) -FlipIt -srcFile matmul.c -config FlipIt.config -singleInj 1 -prob 1e-8 -byte -1 -arith 1 -ctrl 1 -ptr 1 -funcList "" -stateFile "matmul" crpt_matmul.bc -o final.bc
	$(LLVM_BUILD_PATH)/bin/clang  -c final.bc -o matmul.o

main.o: main.c
	$(CC) $(CFLAGS) -o main.o -c main.c



short: _matmul.o _main.o
	$(CC) -o short _matmul.o _main.o $(LFLAGS)

_matmul.o: matmul.c
	$(FLIPIT_PATH)/scripts/flipit-cc $(CFLAGS) -o _matmul.o -c matmul.c

_main.o: main.c
	$(FLIPIT_PATH)/scripts/flipit-cc $(CFLAGS) -o _main.o -c main.c


clean:
	rm -f *.bc
	rm -f *.o
	rm -f *.LLVM.bin
	rm -f *.pyc
	rm -f long
	rm -f short
