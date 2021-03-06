# KoMo2:

## Modernising an Advanced Reduced Instruction Set Computing Machine architecture emulator

###### _A 3<sup>rd</sup> year dissertation project by Lawrence Warren for Dr. Steven Bagley of the University of Nottingham._

---

### A brief history

1<sup>st</sup> year students at _The University of Nottingham_ are taught a compulsory module, _Systems and Architecture (SYS)_, which amongst other things teaches the basics of programming concepts, such as branches and variables, through ARM assembly language.

_KoMoDo_ is an ARM emulator application used for the _SYS_ module, which uses a GUI to display memory addresses, registers, and the values within them at any given point in the FDE cycle of the emulated processor. It also provides buttons to control the operation of the emulated processor: single-step execution; multi-step execution; and breakpoints; amongst other things.

However, _KoMoDo_ is becoming outdated for a number of reasons, and the purpose of the _KoMo2_ project is to provide necessary updates to the applications UI, logic, dependencies, and accessibility.

---

### Installation

1. Install the tools described in the subsection [_Toolchain_](#toolchain).
2. Install the libraries described in the subsection [_Libraries_](#libraries).
3. **Optional** - Install any dependencies you may find in the subsection [_Development environment_](#development-environment).
4. Clone this repository.
5. **Optional** - Execute any scripts found in the `scripts` directory. Information can be found in the subsection [_Shell scripts_](#shell-scripts).
6. Enter the root directory of this project and execute `make`.
7. Run the newly generated `kmd` binary in the `bin` directory.

---

### Dependencies

#### Toolchain

To build this program from source, you **must** have a C and a C++ compiler. The makefile assumes that GCC is installed, and uses the commands `g++` and `gcc`.

Furthermore, to execute the makefile, you must have GNU Make installed.

#### Libraries

_KoMo2_ is implemented using _GTKMM_, a C++ implementation of the popular GUI library _GTK-3_. This in turn has many of it's own dependencies - a full list can be found [here](https://developer.gnome.org/gtkmm-tutorial/stable/sec-installation-dependencies.html.en).

These libraries are popular and compiled versions can likely be found on your Linux distributions package manager. For instance, in the following distributions you can execute the following commands:

_Debian, Gentoo, SuSE & Ubuntu:_
`sudo apt-get install libgtkmm-3.0-dev`

_Fedora, RedHat & CentOS:_
`yum install gtkmm30-docs`

Alternatively, it is possible to compile them from source - instructions are readily available online - but this can be finicky and requires assembling your own toolchain.

#### Development environment

Exactly what text editor or IDE is used for development is up to developer preference, but there are several tools that have been used in initial development of _KoMo2_ to increase code readability and quality:

- **ClangFormat** is a C and C++ formatter that can be run on a source program to create a consistent, human-readable file, and leaves you not having to worry about styling. More information can be found [here](https://clang.llvm.org/docs/ClangFormat.html).

  This program has been developed using the default settings for ClangFormat, however you can configure a custom formatter file.

* **Doxygen** is a tool that allows for generation of documentation on classes, variables and functions from specially styled comments embedded in the code. Many doxygen comments can be found in the code base, but an example of a basic Doxygen comment is as follows:

  ```c
  /**
   * @brief A function which takes a base value and a power, and returns the
   * base to that power. For example, `baseToThePower(2, 2)` returns `4`,
   * `baseToThePower(2, 3)` returns `8`.
   * @param base The base that will be put to the power.
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

---

### Binaries

_KoMo2_ includes a `makefile` in it's root which generates 3 binaries in the `bin` directory when executed using GNU Make. It is always assumed that the 3 binaries exist in the same directory:

##### `bin/kmd`

`kmd` is the main executable _KoMo2_.

Running this binary will launch the GUI, and the following 2 binaries will be forked from it when necessary.

The source for this binary can be found in `src/kmdSrc`.

##### `bin/jimulator`

`jimulator` is the executable for the ARM emulation unit.

This binary is forked at the very beginning of `kmd`'s main function, and the two processes should always run in parallel.

The source code for this binary can be found in `src/jimulatorSrc`.

##### `bin/aasm`

`aasm` is an arm assembler, which takes an input `.s` file and compiles it into an output `.kmd` file, which can be read into `jimulator`.

This binary is forked upon pressing the _"compile & load button"_ present in the GUI, and runs briefly until the output `.kmd` file is generated.

The source code for this executable can be found in `src/aasmSrc`, and compilation is performed in the make file.

##### `bin/mnemonics`

`aasm` also has a plain text file on which it is dependant, `mnemonics`. It is always assumed that `mnemonics` is in the same directory as `aasm`.

---

### Shell scripts

##### install fonts

The shell script `installFonts.sh` has been included in the project `scripts` directory.

_KoMo2_ uses a mono space font family known as [Fira Code](https://github.com/tonsky/FiraCode), and this shell script installs these from an archive file tracked in the `res` directory.

If you enter the `scripts` directory and execute the shell script as root (`sudo ./installFonts.sh`) then these fonts will be installed for you automatically.

**HOWEVER**, it is worth bearing in mind that I found the guide to make this shell script online, and it does move files into a protected directory (specifically `usr/local/share/fonts`) so inspect the shell script for yourself first and make up your mind about if you want to run it.

I have ensured that all fonts used can always fall back to the default "monospace" font option should you not want to install the Fira Code font family.

##### add to path

The shell script in the `scripts` directory, `addToPath.sh`, is WIP!
TODO: MAKE THIS SCRIPT & DESCRIBE IT HERE

---

### User Manual

#### Operating _KoMo2_

_KoMo2_ is simply an interface for _Jimulator_ - if there is some action you would like to perform on _Jimulator_, there is a button in _KoMo2_ for it:

##### Selecting, compiling, and loading an ARM assembly file

You must write your own ARM assembly programs (_.s_ file extension) in an external text editor, and save them to somewhere on your filesystem.

In the top right corner of _KoMo2_ there is a button labelled "Select File", which will open a fill browser that allows you to select any _.s_ file.

Once a file is selected, a label below this button will display the file name and a button below this, labelled "Compile & Load", will create a compiled _.kmd_ file in the same directory, which will subsequently be loaded into _Jimulator_ automatically.

The "Select File" and "Compile & Load" buttons are only accessible while _Jimulator_ is in certain states, and can be executed using the shortcuts **Ctrl+L** and **CTRL+R** respectively.

#### Commencing, pausing, and resuming _Jimulator_ execution

Once a program has been compiled and loaded into _Jimulator_, execution of the program can commence.

A button at the top of the screen with a green play symbol will commence execution of _Jimulator_. Upon clicking this button, the button will display a blue pause symbol. Upon clicking this button execution will pause and the program will pause executing.

The state of the button will change upon every click, alternating between pausing and playing, and the operation it performs will change similarly.

The commence, pause, and play buttons are only accessible when _Jimulator_ is in certain states, and can be executed using the shortcut **F5**.

#### Perform a single _Jimulator_ execution

The single step execution button is only accessible when _Jimulator_ is in certain states, and can be executed using the shortcut **F6**.

#### Halt program execution

The halt execution button is only accessible when _Jimulator_ is in certain states, and can be executed using the shortcut **F1**.

#### Reload the program

The reload program button is only accessible when _Jimulator_ is in certain states, and can be executed using the shortcut **Ctrl+R**.

#### Get help & about _KoMo2_

The about button is accessible in all _Jimulator_ states, and can be executed using the shortcut **F12**.
