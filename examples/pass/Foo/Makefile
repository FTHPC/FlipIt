# Makefile for Foo pass

#####################################################################
#
# This file is licensed under the University of Illinois/NCSA Open 
# Source License. See LICENSE.TXT for details.
#
#####################################################################

CXX=$(LLVM_BUILD_PATH)/bin/clang
LLVM_CONFIG=$(LLVM_BUILD_PATH)/bin/llvm-config

CXXFLAGS= -Wall -O2 -g -std=c++11 -fno-rtti -I$(FLIPIT_PATH)/include
CXXFLAGS += $(shell $(LLVM_CONFIG) --cxxflags) -fPIC 
#CXXFLAGS += -DCOMPILE_PASS 

LDFLAGS = $(shell $(LLVM_CONFIG) --ldflags) -L$(FLIPIT_PATH)/lib -lFlipIt

ifeq ($(shell uname),Darwin)
SHARED_LIB_OPT  = -bundle -undefined dynamic_lookup
else
SHARED_LIB_OPT  = -shared -Wl, -O1
endif

all: libFooPass.so

libFooPass.so:foo.cpp
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $(SHARED_LIB_OPT) $<


clean:
	rm -rf *.o *.so

