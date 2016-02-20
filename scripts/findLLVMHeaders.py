#!/usr/bin/python

#####################################################################
#
# This file is licensed under the University of Illinois/NCSA Open 
# Source License. See LICENSE.TXT for details.
#
#####################################################################

#####################################################################
#
# Name: findLLVMHeaders.py
#
# Description: Searches the $LLVM_REPO_PATH for the location of the 
# 				header files needed by the compiler pass and updates 
#				the '#includes' in the file 'faults.h'. With never 
#				verisons of LLVM these paths may change.
#
#####################################################################

import os
import sys
import subprocess

#get location of LLVM header files
LLVM_BUILD_PATH = os.environ['LLVM_BUILD_PATH']
#LLVM_REPO_PATH = os.environ['LLVM_REPO_PATH']
os.system(LLVM_BUILD_PATH + "/bin/llvm-config --includedir > .tmp.tmp")
LLVM_REPO_PATH = open(".tmp.tmp").read().strip()
os.system("rm .tmp.tmp")

# DOES NOT WORK ON BW
#cmd = [LLVM_BUILD_PATH + "/bin/llvm-config", "--includedir"]
#proc = subprocess.Popen(cmd, shell=True, stdout=subprocess.PIPE)
#LLVM_REPO_PATH = proc.communicate()[0].strip()

if LLVM_REPO_PATH == "":
    LLVM_REPO_PATH = LLVM_BUILD_PATH + "/include/"

# header files used by the complier pass in 'faults.h'
# might should grab this information from 'faults.h' and build at runtime.
flipitHeaders = {'Pass.h': "#include <llvm\/Pass.h>",\
        'ArrayRef.h': "#include <llvm\/ADT\/ArrayRef.h>",\
        'Function.h': "#include <llvm\/IR\/Function.h>",\
        'Module.h': "#include <llvm\/IR\/Module.h>",\
        'User.h': "#include <llvm\/IR\/User.h>",\
        'IRBuilder.h': "#include <llvm\/IR\/IRBuilder.h>",\
        'Instructions.h': "#include <llvm\/IR\/Instructions.h>",\
        'raw_ostream.h': "#include <llvm\/Support\/raw_ostream.h>",\
        'Statistic.h': "#include <llvm\/ADT\/Statistic.h>",\
        #'MachineOperand.h': "#include <llvm\/CodeGen\/MachineOperand.h>",\
        'CommandLine.h': "#include <llvm\/Support\/CommandLine.h>",\
        #'LoopPass.h': "#include <llvm\/Analysis\/LoopPass.h>",\
        'InstIterator.h': "#include <llvm\/IR\/InstIterator.h>",\
        'PassManager.h': "#include <llvm\/PassManager.h>",\
        'CallingConv.h': "#include <llvm\/IR\/CallingConv.h>",\
        'Verifier.h': "#include <llvm\/Analysis\/Verifier.h>",\
        #'PrintModulePass.h': "#include <llvm\/Assembly\/PrintModulePass.h>",\
        'DebugInfo.h': "#include <llvm\/DebugInfo.h>",\
        'Instruction.h': "#include <llvm\/IR\/Instruction.h>",\
        'TypeBuilder.h': "#include <llvm\/IR\/TypeBuilder.h>"}

# replace header files in 'faults.h' with the correct headers for the version 
#of LLVM at $LLVM_REPO_PATH
filePath = sys.argv[1] #os.environ['FLIPIT_PATH'] + "/src/pass/faults.h"
infile= open(filePath)
headerFile = infile.readlines()
infile.close()
for path, subdirs, files in os.walk(LLVM_REPO_PATH + "/llvm"):
    for name in files:
        if name in flipitHeaders.keys():
            newInclude =  "#include <" + os.path.join( path[path.find("include")+8:], name) +">"
            #os.system("sed -i '/" + flipitHeaders[name] + "/c\\" + newInclude + "' " + filePath)
            for i in xrange(len(headerFile)):
                line = headerFile[i]
                if "#include" in line and name in line:
                    headerFile[i] = newInclude +"\n"
                    flipitHeaders.pop(name, None)

# remove the header files for files not found to eliminate compiler errors
if len(flipitHeaders.keys()) > 0:
    print "\n\nWARNING: Unable to locate the following header file(s). " \
            "Compiliation of the compiler pass may not be successful."

    for name in flipitHeaders.keys():
        print  "\t", flipitHeaders[name] 
        #os.system("sed -i '/" +flipitHeaders[name] +"/c\\ ' " + filePath)
        for i in xrange(len(headerFile)):
            line = headerFile[i]
            if "#include" in line and name in line:
                    headerFile[i] = "\n"
    print "\n\n"

outfile = open(filePath, "w")
for i in headerFile:
    outfile.write(i)
outfile.close()

