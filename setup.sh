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


# Setup
echo "Copying files..."
mkdir $LLVM_BUILD_PATH/lib/Transforms/FlipIt/
ln -s -f $FLIPIT_PATH/src/pass/Makefile $LLVM_BUILD_PATH/lib/Transforms/FlipIt/Makefile
ln -s -f $FLIPIT_PATH/src/pass/Makefile-Lib $LLVM_BUILD_PATH/lib/Transforms/FlipIt/Makefile-Lib
ln -s -f $FLIPIT_PATH/src/pass/faults.cpp $LLVM_BUILD_PATH/lib/Transforms/FlipIt/faults.cpp
ln -s -f $FLIPIT_PATH/src/pass/faults.h $LLVM_BUILD_PATH/lib/Transforms/FlipIt/faults.h
ln -s -f $FLIPIT_PATH/scripts/createPass.sh $LLVM_BUILD_PATH/lib/Transforms/FlipIt/createPass.sh

# path could be wrong (create new variables LLVM_BUILD_PATH LLVM_REPO_PATH)
mkdir $LLVM_REPO_PATH/lib/Transforms/FlipIt
ln -s -f $FLIPIT_PATH/src/pass/Makefile $LLVM_REPO_PATH/lib/Transforms/FlipIt/Makefile
ln -s -f $FLIPIT_PATH/src/pass/faults.cpp $LLVM_REPO_PATH/lib/Transforms/FlipIt/faults.cpp
ln -s -f $FLIPIT_PATH/src/pass/faults.h $LLVM_REPO_PATH/lib/Transforms/FlipIt/faults.h
echo "...Done!"

# Reconfigure LLVM
echo "Reconfiguring..."
cd $LLVM_BUILD_PATH
$LLVM_REPO_PATH/configure
echo "Done!"

echo "Creating pass..."
cd $FLIPIT_PATH/scripts/
./findLLVMHeaders.py
cd $LLVM_BUILD_PATH/lib/Transforms/FlipIt
./createPass.sh
echo "Done!"


# Build corrupting library
mkdir $FLIPIT_PATH/lib
echo "Building the corruption library..."
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
