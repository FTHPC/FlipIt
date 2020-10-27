#!/usr/bin/python
#####################################################################
#
# This file is licensed under the University of Illinois/NCSA Open 
# Source License. See LICENSE.TXT for details.
#
#####################################################################

#####################################################################
#
# Name: genBC.py
#
# Description: Generates a bitcode header file that allows multiple 
#		source files to be compiled with FlipIt at the same 
#		time.
#
#####################################################################

import os
import subprocess
import string

FLIPIT_PATH = os.environ['FLIPIT_PATH']
LLVM_BUILD_PATH = os.environ['LLVM_BUILD_PATH']
# compile corrupt.c to LLVM bitcode
os.system(LLVM_BUILD_PATH + "/bin/clang -emit-llvm -c -o tmp.bc " + FLIPIT_PATH +"/src/corrupt/corrupt.c")

#doesn't work correctly on BW
#cmd = [LLVM_BUILD_PATH + "/bin/llvm-link", "-d", "tmp.bc"]
#p = subprocess.Popen(cmd, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
#out, err = p.communicate()
os.system(LLVM_BUILD_PATH + "/bin/llvm-dis tmp.bc -o tmp.ll")
err = open("tmp.ll").read()

# extract the function signitures and other info needed for the header file
# since they are defined in the IR we need to replace define with declare to make a header file
outfile = open(FLIPIT_PATH + "/src/corrupt/corrupt.ll", "w")
attributes = []
for l in err.split("\n"):
    if "define" in l:
        s = l.replace("define", "declare", 1)
        s = s.replace( "internal", "", 1)
        outfile.write(s[0:-2])
        outfile.write("\n")
        a = s[0:-2].split(" ")[-1]
        if a not in attributes:
            attributes.append(a)
    elif "%struct._IO" == l[0:11] or "%struct.__s" == l[0:11]:
        outfile.write(l +"\n")
    elif "target datalayout = " in l or "target triple = " in l:
        outfile.write(l + "\n")
    else:
        for i in attributes:
            if "attributes " +i in l:
                outfile.write(l +"\n")
outfile.close()

os.system(LLVM_BUILD_PATH + "/bin/clang -emit-llvm -c -o " + FLIPIT_PATH + "/src/corrupt/corrupt.bc " + FLIPIT_PATH+"/src/corrupt/corrupt.ll")
os.system("rm tmp.bc tmp.ll")
