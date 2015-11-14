############### Injector Parameters ##################
#
#    config - config file used by the compiler pass
#    funcList - list of functions that are faulty
#    prob - probability that instuction is faulty
#    byte - which byte is faulty (0-7) -1 random
#    singleInj - one injection per active rank (0 or 1)
#    ptr - add code to inject into pointers (0 or 1)
#    arith - add code to inject into mathematics (0 or 1)
#    ctrl - add code to inject into control (0 or 1)
#
#####################################################

config = "jacobi.config"
funcList = "\"\""
prob = 1e-5
byte = -1
singleInj = 1
ptr = 1
arith = 1
ctrl = 1

############# Library Parameters #####################
#
#    FLIPIT_PATH - Path to FlipIt repo
#    SHOW - libraries and path wraped by mpicc 
#
#####################################################
import os
FLIPIT_PATH = os.environ['FLIPIT_PATH'] 
LLVM_BUILD_PATH = os.environ['LLVM_BUILD_PATH'] 
SHOW = ""


########### Files to NOT inject inside ###############
notInject = [" ", " "]

############ Default Compiler #################
cc = "mpicc"

############ Verbose compiler output ##############
verbose = False 

