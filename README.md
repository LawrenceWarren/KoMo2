# KoMo2:

## Modernising an Advanced Reduced Instruction Set Computing Machine architecture emulator

###### _A 3^rd^ year dissertation project by Lawrence Warren for Dr. Steven Bagley of the University of Nottingham._

### A brief history

1^st^ year students at _The University of Nottingham_ are taught a compulsory module, _Systems and Architecture (SYS)_, which, amongst other things, teaches the basics of programming concepts, such as branches and variables, through ARM assembly language.

_KoMoDo_ is an ARM emulator application which uses a GUI to display memory addresses, registers, and the values within them at any given point in the FDE cycle of the emulated processor. However it is becoming outdated for a number of reasons, and is the _KoMo2_ project is about providing the necessary updates.

### Dependencies

_KoMo2_ is implemented using _GTKMM_, a C++ implementation of the popular GUI library _GTK-3_. This in turn has many of it's own dependencies - a full list can be found [here](https://developer.gnome.org/gtkmm-tutorial/stable/sec-installation-dependencies.html.en).

These libraries are popular and can likely be found on your Linux distributions package manager. For instance:

Debian, Gentoo, SuSE & Ubuntu:
`sudo apt-get install libgtkmm-3.0-dev`

Fedora, RedHat & CentOS:
`yum install gtkmm30-docs`

Alternatively, it is possible to compile them from source - instructions are readily available online - but this can be finicky and requires assembling your own toolchain.

### Binaries

_KoMo2_ includes a `makefile` in it's root which generates 3 binaries in the `bin` when executed using the make command. It is always assumed that the 3 binaries exist in the same directory:

#### `./bin/kmd`

`kmd` is the main executable _KoMo2_.

Running this binary will launch the GUI, and the following 2 binaries will be forked from it when necessary.

The source for this binary is any found in `src/kmdSrc`.

#### `./bin/jimulator`

`jimulator` is the executable for the ARM emulation unit.

This binary is forked at the very beginning of `kmd`'s main function, and the two processes should always run in parallel.

The source code for this executable can be found in `./src/jimulatorSrc`.

#### `./bin/aasm`

`aasm` is an arm assembler, which takes an input `.s` file and compiles it into an output `.kmd` file, which can be read into `jimulator`.

This binary is forked upon pressing the _"compile and load button"_ present in the GUI, and runs briefly until the output `.kmd` file is generated.

The source code for this executable can be found in `./src/aasmSrc`, and compilation is performed in the make file.

##### `./bin/mnemonics`

`aasm` also has a plain text file on which it is dependant, `mnemonics`. It is always assumed that `mnemonics` is in the same directory as `aasm`.
