#include <stdio.h>
#include <math.h>
#include <mpi.h>
#include "jacobi.h"
#include "FlipIt/corrupt/corrupt.h"

/* This example handles a 12 x 12 mesh, on 4 processors only. */


int main(int argc, char** argv)
{
	int rank;
	int seed = 233;
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    FLIPIT_Init(rank, argc, argv, time(NULL));
    jacobi(rank);
	FLIPIT_Finalize(NULL);
    
    MPI_Finalize();
    return 0;
}
