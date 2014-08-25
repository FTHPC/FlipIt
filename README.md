FlipIt
======

FlipIt: An LLVM Based Fault Injector for HPC


# FlipIt Installation

1.) Follow Steps from http://clang.llvm.org/get_started.html to install LLVM and Clang

2.) Get FlipIt: git clone git@github.com:aperson40/FlipIt.git

3.) Modify your bashrc file to add directory to path and set some environment variables:

```
 PATH = $PATH:/path/to/build/Debug+Asserts/bin
 export PATH
 export LLVM_BUILD_PATH=/path/to/build/
 export LLVM_REPO_PATH=/path/to/repo/llvm/
 export FLIPIT_PATH=/path/to/flipit
```

4.) Run script to setup FlipIt with LLVM (Note: If you're not using LLVM version 3.4, the LLVM #include files in [faults.h](src/pass/faults.cpp) might not be correct. This is due to changes in the directory structure in LLVM between releases.):

```
cd $FLIPIT_PATH/
./setup
```
5.) Test that everything works:


```
cd $FLIPIT_PATH/examples
./testPass.sh
```

If all worked correctly, 2 + 2 is not 4.

------------------------------
FlipIt is licensed under the University of Illinois/NCSA Open Source License. See [LICENSE.TXT](LICENSE.TXT) for details.
