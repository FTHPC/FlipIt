FlipIt
======

FlipIt: An LLVM Based Fault Injector for HPC

To cite:
```
@inproceedings{DBLP:conf/europar/CalhounOS14,
  author    = {Jon Calhoun and Luke Olson and Marc Snir},
  title     = {FlipIt: An {LLVM} Based Fault Injector for {HPC}},
  booktitle = {Euro-Par 2014: Parallel Processing Workshops - Euro-Par 2014 International Workshops, Porto, Portugal, August 25-26, 2014, Revised Selected Papers, Part {I}},
  pages     = {547--558},
  year      = {2014},
  crossref  = {DBLP:conf/europar/2014w1},
  url       = {http://dx.doi.org/10.1007/978-3-319-14325-5_47},
  doi       = {10.1007/978-3-319-14325-5_47},
  timestamp = {Sun, 14 Dec 2014 18:38:30 +0100},
  biburl    = {http://dblp.uni-trier.de/rec/bib/conf/europar/CalhounOS14},
  bibsource = {dblp computer science bibliography, http://dblp.org}
}
```
This project is partially supported by the Air Force Office of Scientific Research under grant FA9550-12-1-0478.


# FlipIt Installation

1.) Follow Steps from http://clang.llvm.org/get_started.html to install LLVM and Clang

2.) Get FlipIt: 

```
git clone git@github.com:aperson40/FlipIt.git
```

3.) Modify your bashrc file to add directory to path and set some environment variables. If you have a *Release+Asserts* build of LLVM, replace *Debug+Asserts* with it in the following:

```
 PATH=$PATH:/path/to/build/Debug+Asserts/bin
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

If all worked correctly, 2 + 2 is not 4. If not, check out the [Examples](https://github.com/aperson40/FlipIt/wiki/Examples) wiki page for suggested solutions.

Detailed walk though of the FlipIt [examples](https://github.com/aperson40/FlipIt/wiki/Examples).

------------------------------
FlipIt is licensed under the University of Illinois/NCSA Open Source License. See [LICENSE.TXT](LICENSE.TXT) for details.
