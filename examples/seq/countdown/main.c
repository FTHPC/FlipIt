#include <stdio.h>
#include "FlipIt/corrupt/corrupt.h"

int main(int argc, char** argv)
{
	int n = 100, numIncorrect = 0, seed = 533, i, j;
	double* a = (double*) malloc(n*n*sizeof(double));
	double* b = (double*) malloc(n*n*sizeof(double));
	double* c = (double*) malloc(n*n*sizeof(double));
	double* c_golden = (double*) malloc(n*n*sizeof(double));

	/* Initialize arrays */
	for (i=0; i<n; i++)
		for(j=0; j<n; j++)
		{
			a[i*n + j] = i*j;
			b[i*n + j] = i*j;
			c[i*n + j] = 0;
			c_golden[i*n + j] = 0;
		}
	/* Set up injector and compute golden solution not in MPI so 
	    pass 0 for the first argument(MPI rank)*/
	FLIPIT_Init(0, argc, argv, seed);

    /* Use a countdown timer to inject after 125 instructions.
        Compile with -singleInj 0 to inject after every 125 instructions. */
    FLIPIT_CountdownTimer(125);
	FLIPIT_SetInjector(FLIPIT_OFF);
	matmul(a, b, c_golden, n);

	/* compute */
	printf("Starting faulty computation.\n");
	FLIPIT_SetInjector(FLIPIT_ON);
	matmul(a, b, c, n);

	/* check */
	for (i=0; i<n; i++)
		for (j=0; j<n; j++)
			if (c[i*n + j] != c_golden[i*n +j])
				numIncorrect += 1;

	printf("Number of incorrect elements: %d\n", numIncorrect);
	FLIPIT_Finalize(NULL);

	return 0;
}
