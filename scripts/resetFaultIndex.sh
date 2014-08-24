#!/bin/bash

#####################################################################
#
# This file is licensed under the University of Illinois/NCSA Open 
# Source License. See LICENSE.TXT for details.
#
#####################################################################

#####################################################################
#
# Name: resetFaultIndex.sh
#
# Description: Resets the fault site indexing- i.e., the next time 
# FlipIt complies code it will start with fault index 0, 1, 2, ...
#
#####################################################################

if [ [ -a ~/.FlipItState] ]
	then
	rm ~/.FlipItState
fi
