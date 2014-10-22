# Makefile for hello pass

# Path to top level of LLVM hierarchy
LEVEL = ../../..

# Name of the library to build
LIBRARYNAME = Foo

# Make the shared library become a loadable module so the tools can
# dlopen/dlsym on the resulting library.
LOADABLE_MODULE = 1
#SHARED_LIBRARY = 1
#BUILD_ARCHIVE = 1

# Include the makefile implementation stuff
include $(LEVEL)/Makefile.common

# Add linkage such that Foo can call FlipIt to corrupt instructions
LIBS += -L /home/aperson40/research/build//Debug+Asserts/lib/ -lFlipIt
