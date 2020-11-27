# KoMo2:

## Modernising an Advanced Reduced Instruction Set Computing Machine architecture emulator

###### _A 3^rd^ year dissertation project by Lawrence Warren for Doctor Steven Bagley PhD. of the University of Nottingham._

### Dependencies

_KoMo2_ is implemented using _GTKMM_, a C++ implementation of the popular GUI library _GTK-3_. This in turn has many of it's own dependencies - a full list can be found [here](https://developer.gnome.org/gtkmm-tutorial/stable/sec-installation-dependencies.html.en).

These libraries are popular and can likely be found on your Linux distributions package manager. However, it is possible to compile them from source - instructions are readily available online.

_KoMo2_ also uses two binaries:

- `./bin/aasm`, which takes a target `.s` file and compiles it to a `.kmd`. The source code for this executable can be found in `./src/aasmSrc`, and compilation is performed in the make file.

  - `./bin/aasm` depends on a file, `./bin/mnemonics`, which must be in the same directory as `./bin/aasm`.

- `./bin/Jimulator`, which is the main ARM emulation program. The source code for this executable can be found in `./src/jimulatorSrc`, and compilation is performed in the make file.
