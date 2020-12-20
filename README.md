# KoMo2:

## Modernising an Advanced Reduced Instruction Set Computing Machine architecture emulator

###### _A 3<sup>rd</sup> year dissertation project by Lawrence Warren for Dr. Steven Bagley of the University of Nottingham._

### A brief history

1<sup>st</sup> year students at _The University of Nottingham_ are taught a compulsory module, _Systems and Architecture (SYS)_, which, amongst other things, teaches the basics of programming concepts, such as branches and variables, through ARM assembly language.

_KoMoDo_ is an ARM emulator application which uses a GUI to display memory addresses, registers, and the values within them at any given point in the FDE cycle of the emulated processor. However it is becoming outdated for a number of reasons, and is the _KoMo2_ project is about providing the necessary updates.

### Dependencies

##### Libraries

_KoMo2_ is implemented using _GTKMM_, a C++ implementation of the popular GUI library _GTK-3_. This in turn has many of it's own dependencies - a full list can be found [here](https://developer.gnome.org/gtkmm-tutorial/stable/sec-installation-dependencies.html.en).

These libraries are popular and compiled versions can likely be found on your Linux distributions package manager. For instance, on the following distributions you can execute the respect command:

_Debian, Gentoo, SuSE & Ubuntu:_
`sudo apt-get install libgtkmm-3.0-dev`

_Fedora, RedHat & CentOS:_
`yum install gtkmm30-docs`

Alternatively, it is possible to compile them from source - instructions are readily available online - but this can be finicky and requires assembling your own toolchain.

##### Toolchain

To build this program from source, you **must** have a C and a C++ compiler. The makefile assumes that GCC is installed, and uses the commands `g++` and `gcc`.

Furthermore, to execute the makefile, you must have GNU Make installed.

##### Development environment

Exactly what text editor or IDE is used for development is up to developer preference, but there are several tools that have been used in initial development of the program to increase code readability and quality:

- **ClangFormat** is a C and C++ formatter that can be run on a source program to create a consistent, human-readable file, and leaves you not having to worry about styling. More information can be found [here](https://clang.llvm.org/docs/ClangFormat.html).

  This program has been developed using the default settings for ClangFormat, however you can configure a custom formatter file.

* **Doxygen** is a tool that allows for generation of documentation on classes, variables and functions from specially styled comments embedded in the code. Many doxygen comments can be found in the code base, but an example of a basic Doxygen comment is as follows:

  ```c
  /**
   * @brief A function which takes a base value and a power, and returns the
   * power to that base. For example, `baseToThePower(2, 2)` returns `4`,
   * `baseToThePower(2, 3)` returns `8`.
   * @param base The number that will be put to the power.
   * @param power The power to raise the base by.
   * @returns int - the resulting value of the base to the power.
   */
  int baseToThePower(int base, int power) {
    if(!power) {
      return 1;
    }

    return base * baseToThePower(base, power - 1);
  }
  ```

  More information about Doxygen can be found [here](https://www.doxygen.nl/index.html).

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
