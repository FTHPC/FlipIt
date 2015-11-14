# Makefile for faults pass

#####################################################################
#
# This file is licensed under the University of Illinois/NCSA Open 
# Source License. See LICENSE.TXT for details.
#
#####################################################################

CXX=$(LLVM_BUILD_PATH)/bin/clang
LLVM_CONFIG=$(LLVM_BUILD_PATH)/bin/llvm-config

CXXFLAGS= -Wall -O2 -g -std=c++11 -fno-rtti
CXXFLAGS += $(shell $(LLVM_CONFIG) --cxxflags) -fPIC 
#CXXFLAGS += -DCOMPILE_PASS 

LDFLAGS = $(shell $(LLVM_CONFIG) --ldflags)

ifeq ($(shell uname),Darwin)
SHARED_LIB_OPT  = -bundle -undefined dynamic_lookup
else
SHARED_LIB_OPT  = -shared -Wl, -O1
endif

all: libFlipItPass.so libFlipIt.so

libFlipItPass.so:faults.cpp
	$(CXX) $(CXXFLAGS) -DCOMPILE_PASS $(LDFLAGS) -o $@ $(SHARED_LIB_OPT) $<

libFlipIt.so:faults.cpp
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $(SHARED_LIB_OPT) $<

clean:
	rm -rf *.o *.so
