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
# Description: Compiles the LLVM compiler pass Foo that calls Flipit
#              from inside the pass to corrupt certain instructions.
#
#####################################################################

make clean
make -f Makefile

