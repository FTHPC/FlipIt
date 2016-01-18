#!/bin/bash

#####################################################################
#
# This file is licensed under the University of Illinois/NCSA Open 
# Source License. See LICENSE.TXT for details.
#
#####################################################################

#####################################################################
#
# Name: setupPassExample.sh
#
# Description: Compiles the LLVM compiler pass Foo that calls Flipit
#              from inside the pass to corrupt certain instructions
#              and runs an example to demonstrate it works.
#
#####################################################################

cd $FLIPIT_PATH/scripts/
./findLLVMHeaders.py $FLIPIT_PATH/examples/pass/Foo/foo.h
cd $FLIPIT_PATH/examples/pass/Foo

./createPass.sh
./test.sh
