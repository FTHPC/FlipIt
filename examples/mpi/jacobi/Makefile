#####################################################################
#
# This example shows how to used command line arguments to inject 
# on a single rank. In addition, we use a custom config file located 
# in this directoy "flipit/examples/mpi/jacobi/config.py" to change 
# the default parameters for the compiler pass.
#
#####################################################################
CC=$(FLIPIT_PATH)/scripts/flipit-cc
CFLAGS = -c -g

FILIB = -L$(FLIPIT_PATH)/lib -lcorrupt
LFLAGS = $(FILIB) -lm

jacobi: jacobi.o main.o
	mpicc -o jacobi jacobi.o main.o $(LFLAGS)

jacobi.o: jacobi.c
	$(CC) $(CFLAGS) -o jacobi.o jacobi.c

main.o: main.c
	$(CC) $(CFLAGS) -o main.o main.c

clean:
	rm *.bc
	rm *.LLVM.txt
	rm *.o
	rm jacobi

test-selection:
	mpirun -n 4 ./jacobi --numberFaulty 1 --faulty 3

test-all:
	mpirun -n 4 ./jacobi
