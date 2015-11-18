This example demonstrates how to use the visualization scripts found in
`$FLIPIT_PATH/scripts/analysis` on the Jacobi example. Before we visualize
anything we first need to run the application compiled with FlipIt several
times to get significant enough data. Here we only run the application 25
times which is not sufficient for publication quality results, but is sufficient
to demonstrate how to utilize the analysis scripts.

```bash
#compile the application
make

#make a directory for the run output and run jacobi 25 times
mkdir run_data
for $i in [0..25]
do
	#in a real run we would want to set the faulty rank a random
    mpirun -n 4 ./jacobi --numberFaulty 1 --faulty 3 1&2> run_$i.txt
enddo
```

Now that we have run data, we are able to visualize. We recommend that
you copy analysis scripts into your project's directory as we will be modifying
several files.

```bash
cp -r $FLIPIT_PATH/scripts/analysis .
```

The file `analysis_config.py` has several variables that need to be set
to correctly visualize the fault injection campaign.


* LLVM_log_path 
* trial_path 
* trial_prefix 
* srcPath 
* numTrials 
* more_detail_funcs 
* busError 
* assertMessage 
* segError 
* detectMessage 

Once these are set, we can visualize the campaign

```bash
python main.py
```


FlipIt's analysis scripts allow the graphing of user logged data.

```bash
mkdir run_data_custom
for $i in [0..25]
do
	#in a real run we would want to set the faulty rank a random
    mpirun -n 4 ./jacobi-custom --numberFaulty 1 --faulty 3 1&2> run_$i.txt
enddo
```

If we look at the difference bettwen the two programs, we see the addition of the following code:

```c
int ITER
```
