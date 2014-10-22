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

cp -r Foo   $LLVM_BUILD_PATH/lib/Transforms/
cp -r Foo   $LLVM_REPO_PATH/lib/Transforms/
$LLVM_REPO_PATH/configure
cd $LLVM_BUILD_PATH/lib/Transforms/Foo
./createPass.sh

./test.sh
