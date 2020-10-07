# Makefile for faults pass

#####################################################################
#
# This file is licensed under the University of Illinois/NCSA Open 
# Source License. See LICENSE.TXT for details.
#
#####################################################################

#CXX=$(LLVM_BUILD_PATH)/bin/clang
CXX=g++
LLVM_CONFIG=$(LLVM_BUILD_PATH)/bin/llvm-config

CXXFLAGS= -Wall -O2 -g -std=c++11 -fno-rtti -I$(FLIPIT_PATH)/include
CXXFLAGS += $(shell $(LLVM_CONFIG) --cxxflags) -fPIC 
#CXXFLAGS += -DCOMPILE_PASS 

GCC_MAJOR=$(shell gcc -v  2>&1 /dev/null | grep -i "gcc version" | awk -F" " '{split($$3, a, ".")} END{print a[1]}')

ifneq ($(GCC_MAJOR),4)
	CXXFLAGS += -DUSE_OLD_ABI=1
endif

LDFLAGS = $(shell $(LLVM_CONFIG) --ldflags)

ifeq ($(shell uname),Darwin)
SHARED_LIB_OPT  = -bundle -undefined dynamic_lookup
else
SHARED_LIB_OPT  = -shared 
endif

#all: libFlipItPass.so
all: libFlipItPass.so libFlipIt.so

libFlipItPass.so:faults.cpp
#libFlipItPass.so:faults_new.cpp
	$(CXX) $(CXXFLAGS) -DCOMPILE_PASS $(LDFLAGS) -o $@ $(SHARED_LIB_OPT) $<

libFlipIt.so:faults.cpp
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $(SHARED_LIB_OPT) $<

clean:
	rm -rf *.o *.so
