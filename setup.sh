#!/bin/bash

#####################################################################
#
# This file is licensed under the University of Illinois/NCSA Open 
# Source License. See LICENSE.TXT for details.
#
#####################################################################

#####################################################################
#
# Name: setup.sh
#
# Description: Sets up and and reconfigures existing LLVM build, 
#				$LLVM_PATH, with the fault injector FlitIt, $FLIPIT_PATH.
#
#####################################################################
# setup.sh


echo "

Creating pass..."
cd $FLIPIT_PATH/scripts/
./findLLVMHeaders.py $FLIPIT_PATH/src/pass/faults.h
#cd $LLVM_BUILD_PATH/lib/Transforms/FlipIt
#./createPass.sh


echo "

Copying headers to include dir"
cd $FLIPIT_PATH
mkdir -p -v include/FlipIt/corrupt
mkdir -p -v include/FlipIt/pass
cp src/pass/faults.h include/FlipIt/pass/
cp src/pass/Logger.h include/FlipIt/pass/

cd $FLIPIT_PATH/src/pass
make -f Makefile
mkdir $FLIPIT_PATH/lib/
mv *.so $FLIPIT_PATH/lib
echo "Done!"

# Modify examples have correct #inlcude "corrupt.h"
echo "

Updating headers in examples..."
cd $FLIPIT_PATH
sed -i '/#include "\/path\/to\/flipit\/src\/corrupt\/corrupt.h"/c\#include "'$FLIPIT_PATH'/src/corrupt/corrupt.h"'  ./examples/seq/matmul/main.c
sed -i '/#include "\/path\/to\/flipit\/src\/corrupt\/corrupt.h"/c\#include "'$FLIPIT_PATH'/src/corrupt/corrupt.h"'  ./examples/mpi/jacobi/main.c
sed -i '/#include "\/path\/to\/flipit\/src\/corrupt\/corrupt.h"/c\#include "'$FLIPIT_PATH'/src/corrupt/corrupt.h"'  ./examples/pass/Foo/main.c
echo "Done!"

# Build corrupting library
#mkdir $FLIPIT_PATH/lib
echo "

Building the corruption library..."
cd $FLIPIT_PATH/scripts/
./library.sh
./genBC.py



if [[ -a libcorrupt.a ]]; then
	cp libcorrupt.a $FLIPIT_PATH/lib/
	# copy the library to a location that is in the library path
	if [ "$(whoami)" != "root" ]; then
		echo "Warning: Unable to copy corruption library to '/usr/local/lib'"
		echo ""
		echo "You can link to the corruption library using:"
	else
		cp libcorrupt.a /usr/local/lib
		echo "You can link to the corruption library using:"
		echo "    -lcorrupt"
	fi
	rm libcorrupt.a

	echo "    -L$FLIPIT_PATH/lib -lcorrupt"
else
	echo "Error: Unable to make corruption library!"
fi

echo "

Copying headers to include dir"
cd $FLIPIT_PATH
cp src/corrupt/*.h include/FlipIt/corrupt/
cp src/corrupt/corrupt.bc include/FlipIt/corrupt/
