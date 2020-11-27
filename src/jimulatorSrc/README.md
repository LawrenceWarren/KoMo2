# Jimulator

## Introduction

_KoMo2_, and its predecessor _KoMoDo_, are both ARM emulation GUI's. _Jimulator_, therefore, is the ARM emulation program which _KoMoDo_ was built around, and is therefore the basis for _KoMo2_.

## Credits

The original source found in this directory is the original work of _[Jim Garside](http://apt.cs.manchester.ac.uk/people/jgarside/)_ for _The University of Manchester_.

The documentation page for _Jimulator_ and _KoMoDo_ can be found [at this site.](http://studentnet.cs.manchester.ac.uk/resources/software/komodo/)

## Building

If you have all of the dependencies of _KoMo2_ installed - that is, the GTKMM library and all of its child dependencies - then you have the resources you need to build Jimulator.

From the project root, execute:

    gcc `pkg-config --cflags gtkmm-3.0` -o bin/jimulator src/jimulatorSrc/jimulator.c `pkg-config --libs gtkmm-3.0` -O2 -Wall

Alternatively, the makefile in the project root will build both _KoMo2_ and _Jimulator_ together.

## Architecture

The source `.h` and `.c` files present in this directory have not been altered during the creation of _KoMo2_, as of 27/11/2020.

As such, they are written in a fairly outdated way, with sprawling header files filled with global variables. They also depend on a C compiler to be built (which you should have if you can compile C++)

The _Jimulator_ executable is run via a call to `fork()` and communicates with _KoMoDo_ and _KoMo2_ using Unix pipes.
