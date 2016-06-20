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
#		linked against. This library holds the user accessable 
#		functions of FlipIt as well as the internal functions 
#		that flip bits.
#
#####################################################################


# Without Histogram
if [[ -e $FLIPIT_PATH/lib/libcorrupt.a ]]
 	then
	rm $FLIPIT_PATH/lib/libcorrupt.a
fi

gcc -O3 -c $FLIPIT_PATH/src/corrupt/corrupt.c
ar -cvq libcorrupt.a corrupt.o
if [[ -e corrupt.o ]]; then
	rm corrupt.o
fi


# With Histogram
if [[ -e $FLIPIT_PATH/lib/libcorrupt_histo.a ]]
 	then
	rm $FLIPIT_PATH/lib/libcorrupt_histo.a
fi

gcc -O3 -DFLIPIT_HISTOGRAM -c $FLIPIT_PATH/src/corrupt/corrupt.c -o corrupt_histogram.o
ar -cvq libcorrupt_histo.a corrupt_histogram.o
if [[ -e corrupt_histogram.o ]]; then
	rm corrupt_histogram.o
fi
