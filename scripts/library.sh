#!/bin/bash

#####################################################################
#
# This file is licensed under the University of Illinois/NCSA Open 
# Source License. See LICENSE.TXT for details.
#
#####################################################################

#####################################################################
#
# Name: library.sh
#
# Description: Generates the corruption library that the program is 
#				linked against. This library holds the user accessable 
#				functions of FlipIt as well as the internal functions 
#				that flip bits.
#
#####################################################################

if [[ -a $FLIPIT_PATH/lib/libcorrupt.a ]]
 	then
	rm $FLIPIT_PATH/lib/libcorrupt.a
fi

gcc -O3 -c $FLIPIT_PATH/src/corrupt/corrupt.c
ar -cvq libcorrupt.a corrupt.o


if [[ -a corrupt.o ]]; then
	rm corrupt.o
fi