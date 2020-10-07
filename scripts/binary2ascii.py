#!/usr/bin/python
#####################################################################
#
# This file is licensed under the University of Illinois/NCSA Open 
# Source License. See LICENSE.TXT for details.
#
#####################################################################

#####################################################################
#
# Name: binary2ascii.py
#
# Description: Converts a binary log file into a human readable ASCII
#       format. You can use the -o argument to change the output file
#       name from foo.LLVM.txt to a user provied name.
#
#       e.g. binary2ascii.py foo.LLVM.bin -o bar.LLVM.txt
#
#####################################################################

import sys
import os

# get binary parsing functions from the analysis scripts
FLIPIT_PATH = os.environ['FLIPIT_PATH']
sys.path.insert(0, FLIPIT_PATH + "/scripts/analysis")
from binaryParser import *

#parse arguments
outfile = ""
if "-o" in sys.argv:
    idx = sys.argv.index("-o")
    if idx+1 >= len(sys.argv):
        print ("Unknown output file name.")
        sys.exit(1)
    outfile = sys.argv[idx + 1]

infile = sys.argv[1]
if not os.path.isfile(infile):
    print ("File not found", infile)
    sys.exit(1)

parseBinaryLogFile(None, infile, outfile)
