#!/bin/bash

#####################################################################
#
# This file is licensed under the University of Illinois/NCSA Open 
# Source License. See LICENSE.TXT for details.
#
#####################################################################

#####################################################################
#
# Name: createPass.sh
#
# Description: Compiles the LLVM compiler pass of FlipIt
#
#####################################################################

make clean
make
g++ -shared ./Debug+Asserts/faults.o -o ./Debug+Asserts/faults.so

