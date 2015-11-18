#!/bin/bash


#make a directory for the run output and run jacobi 25 times
mkdir run_data
cd run_data
for i in {0..25}
do
	#in a real run we would want to set the faulty rank a random
    echo $i
    mpirun -n 4 ../jacobi --numberFaulty 1 --faulty 3 &> run_$i.txt
done
