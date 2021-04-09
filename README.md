# _KoMo2_:

## Modernising an Advanced Reduced Instruction Set Computing Machine architecture emulator

###### A 3<sup>rd</sup> year dissertation project by Lawrence Warren for Dr. Steven Bagley of _The University of Nottingham_.

##### Table of Contents

- [What is _KoMo2_? A brief history](#what-is-komo2-a-brief-history)
- [Installation](#installation)
- [Dependencies](#dependencies)
- [User Manual](#user-manual)
- [Binaries](#binaries)
- [Shell Scripts](#shell-scripts)
- [Contributing to _KoMo2_](#contributing-to-komo2)

---

### What is _KoMo2_? A brief history

1<sup>st</sup> year students at _The University of Nottingham_ are taught a compulsory module, _Systems and Architecture (SYS)_, which teaches them the fundamentals of a CPU architecture such as registers, memory, and assembly instructions. To do this, students are tasked with writing their own basic ARM assembly programs and running them in a program called _KoMoDo_.

_KoMoDo_ is an ARM emulator application used for the _SYS_ module, which uses a GUI to display the values within the memory and registers of an emulated ARM computer system.

_KoMoDo_ provides buttons to control the operation of the emulated processor:

- Perform a single FDE cycle.
- Begin an indefinite FDE loop.
- Pause an indefinite FDE loop.
- Refresh the system (reload the program, set _Program Counter_ to 0)

It also allows for the setting of breakpoints, which will interrupt an indefinite FDE loop if the PC steps into a specified memory address.

However, _KoMoDo_ is becoming outdated. The purpose of the _KoMo2_ project is to provide necessary updates to the applications UI, logic, dependencies, and accessibility, while maintaining _KoMoDo_'s functionality and ease of use.

**In Short:** _KoMo2_ is a Linux GUI application that allows for both the inspection and control of the state of an emulated ARM computer system, which can load and run user written ARM assembly programs.

---

### Installation

1. Install the required tools described in the subsection [_Toolchain_](#toolchain).
2. Install the required libraries described in the subsection [_Libraries_](#libraries).
3. Clone this repository.
4. Enter the root directory of this project.
5. Run the `make` command to execute the provided makefile.
6. **Optional** - Execute any scripts found in the `scripts` directory. Information can be found in the subsection [_Shell scripts_](#shell-scripts).
7. Run the newly generated `kmd` binary in the `bin` directory.

---

### Dependencies

#### Toolchain

To build this program from source, you must have a C and a C++ compiler. The makefile assumes that GCC is installed, and uses the commands `g++` and `gcc`.

Furthermore, to execute the makefile, you must have GNU Make installed, giving you access to the command `make`.

#### Libraries

_KoMo2_ is implemented using _GTKMM_, a C++ implementation of the popular GUI library _GTK-3_. This in turn has many of it's own dependencies - a full list can be found [here](https://developer.gnome.org/gtkmm-tutorial/stable/sec-installation-dependencies.html.en).

These libraries are popular and compiled versions can likely be found on your Linux distributions package manager. For instance, in the following distributions you can execute the following commands:

_Debian, Gentoo, SuSE & Ubuntu:_
`apt-get install libgtkmm-3.0-dev`

_Fedora, RedHat & CentOS:_
`yum install gtkmm30-docs`

Alternatively, it is possible to compile them from source - instructions are readily available online - but this can be finicky and requires assembling your own toolchain.

---

### User Manual

#### Operating _KoMo2_

Below is a clear description on how to utilise _KoMo2_.

##### Selecting, compiling, and loading an ARM assembly file

You must write your own ARM assembly programs (`.s` file extension) in an external text editor, and save them on your filesystem.

In the top right corner of _KoMo2_ there is a button labelled "Select File", which will open a file browser that allows you to select any `.s` file on your system.

Once a file is selected, press the button labelled "Compile & Load", which will create a compiled `.kmd` file in the same directory as your `.s` file, and will automatically load this into the ARM emulator.

The "Select File" and "Compile & Load" buttons are only accessible while _KoMo2_ is in certain states, and can be executed using the shortcuts **Ctrl+L** and **CTRL+R** respectively.

##### Commencing, pausing, and resuming execution

Once a program has been compiled and loaded into the ARM emulator, execution of the program can commence.

There is a button on the bar at the top of the screen which displays a green play symbol. Upon clicking this button, the program loaded will begin executing and the button will toggle to display a blue pause symbol. Upon pressing again, the program loaded will pauses execution and the button will toggle back to a green play symbol.

The state of the button will change upon every click, alternating between pausing and playing, and the operation it performs will change similarly between commencing, pausing, and resuming execution.

The commence, pause, and play buttons are only accessible when _KoMo2_ is in certain states, and can be executed using the shortcut **F5**.

##### Perform a single-step execution

There is a button on the bar at the top of the screen which displays an arcing blow arrow. Upon pressing this button, the ARM emulator will perform perform a single step of execution - whatever instruction is at the memory address indicated by the value in the Program Counter register will be executed - and then stop.

The single step execution button is only accessible when _KoMo2_ is in certain states, and can be executed using the shortcut **F6**.

##### Halt program execution

There is a button on the bar at the top of the screen which displays a hollow red square. Upon pressing this button, the ARM emulator will be set to a halted state - execution will stop, and will not be allowed to be resumed again, unless the program is recompiled and loaded.

The halt execution button is only accessible when _KoMo2_ is in certain states, and can be executed using the shortcut **F1**.

##### Reload the program

There is a button on the bar at the top of the screen which displays a blue arrow pointing to it's own tail in a circular shape. Upon pressing this button, the ARM emulator will reset itself - execution will stop and the Program Counter will return to 0, meaning that execution will begin again as if the program was running for the first time.

The reload program button is only accessible when the _KoMo2_ is in certain states, and can be executed using the shortcut **Ctrl+R**.

#### _KoMo2_ GUI elements

Below is a clear description of the remaining _KoMo2_ GUI elements, and what each of them represents and does.

##### The register values

There is a table of 16 rows and 2 columns on the left hand side of the _KoMo2_ GUI. This table displays all of the ARM emulator's CPU registers, and the values within them.

The first 15 registers - labelled `R0` through `R14` - are the values within the general purpose registers of the CPU.

The final register - labelled `PC` - is the Program Counter, which contains the memory address of the next instruction to execute. For each cycle of the ARM CPU, this will change (it will usually increase linearly, but may jump up or down values if meeting a branch instruction)

If the address within the PC is currently visible in the memory window, it will be highlighted in the memory window.

As the CPU runs, the values within all of these registers may change depending on how your program utilises them.

##### The memory window

The large scrolling window that takes up the majority of the _KoMo2_ GUI is the memory window - this displays what is loaded into the ARM emulator's memory at the time, and at what address.

You can scroll up and down this window to view a wide range of memory addresses - the ARM emulators address bus is 32-bit.

As you look at the memory window, you can see red buttons on each row. These buttons can be toggled on or off to set breakpoints within the ARM emulator, which will pause execution of the program if the Program Counter reaches that memory address.

##### The terminal

As _KoMo2_ performs actions, it may log some outputs. These outputs are viewable in the terminal, which takes up the majority of the bottom of the _KoMo2_ GUI. The contents of the terminal can be cleared through a nearby button labelled "Clear".

If you write a program which requests input, you may utilise the singular input box below the terminal window to provide this input. Text will not show in the input box - rather it is immediately captured and sent to ARM emulator, which will process the input and display some output into the terminal window in response.

#### Screen reader support

As part of ensuring _KoMo2_ is accessible for all who use it, the Accessibility ToolKit (ATK) API has been implemented within the program to allow for screen reader compatibility.

If you are using a compatible screen reader - _KoMo2_'s screen reader implementation was tested with [Orca](https://help.gnome.org/users/orca/stable/) - information will be read out as you navigate over GUI elements. For example, if you navigate over a memory row, it will state what address it is, what is stored at that address, and if a breakpoint is set.

For reading the register values, which are not keyboard navigable, press the Alt key + 0-9, A-E to read the relevant register values. For example, _Alt+0_ reads the value stored in register 0, _Alt+8_ reads the value in register 8, _Alt+A_ reads the value in register 10, and _Alt+E_ reads the value in register 14.

For reading the Program Counter, press _Alt+P_.

Furthermore, pressing _Alt+M_ toggles how the memory window mnemonics are read out. By default, the ARM instructions in the memory window are read it as they are displayed on the screen. However, this may not be the best way to communicate what they do to somebody who is hard of sight. Toggling the mnemonics mode allows for the mnemonics to be converted into English and read out that way - they are **not** converted in how they are displayed in the GUI.

### Binaries

_KoMo2_ includes a `makefile` in it's root which generates 3 binaries in the `bin` directory. It is always assumed that the 3 binaries exist in the same directory, and an additional plaintext file:

#### `bin/kmd`

`kmd` is the executable for the _KoMo2_ program proper.

Running this binary will launch the GUI, and the following 2 binaries will be forked from it when necessary.

The source files for this binary can be found in the directory `src/kmdSrc/`.

#### `bin/jimulator`

`jimulator` is the executable for the ARM emulation program _Jimulator_, for which _KoMo2_ is a front end.

This binary is forked at the very beginning of `kmd`'s main function, and the two processes should always run in parallel.

The source files for this binary can be found in `src/jimulatorSrc/`.

#### `bin/aasm`

`aasm` is the executable for an arm assembler program, which takes an input `.s` ARM source file and compiles it into a proprietary _Jimulator_ readable `.kmd` file, which can be loaded into the `jimulator` binary.

This binary is forked upon pressing the _"compile & load button"_ present in the GUI, and runs briefly until the output `.kmd` file is generated.

The source files for this executable can be found in `src/aasmSrc/`, and compilation is performed in the make file.

#### `bin/mnemonics`

`mnemonics` is a plain text file on which `aasm` is dependant. It is **always** assumed that `mnemonics` is in the same directory as the `aasm` binary.

---

### Shell Scripts

A shell script is provided with _KoMo2_ that allow for an enhanced user experience.

#### Custom fonts

The shell script `installFonts.sh` has been included in the project `scripts` directory.

_KoMo2_ uses a mono space font family known as [Fira Code](https://github.com/tonsky/FiraCode), and this shell script installs these from an archive file tracked in the `res/` directory.

If you enter the `scripts/` directory and execute the shell script as root (`sudo ./installFonts.sh`) then these fonts will be installed for you automatically.

**WARNING**, it is worth bearing in mind that the shell script provided is based on one found in [this Medium article](https://medium.com/source-words/how-to-manually-install-update-and-uninstall-fonts-on-linux-a8d09a3853b0), and it require root privileges as it does access the protected directory `usr/local/share/fonts`, so inspect the shell script and linked article first and make up your mind about if you want to run it.

I have ensured that all fonts used can always fall back to the default "monospace" font option should you not want to install the Fira Code font.

---

### Contributing to _KoMo2_

Contributing to _KoMo2_ is easy! If you identify a flaw with the program, or have an idea for a feature request, you are invited to attempt to implement it yourself on a fork of the existing repository, or [open an issue on the GitHub](https://github.com/LawrenceWarren/KoMo2/issues) to discuss it further.

Some guidance below is provided on how to get started.

#### Development environment

Exactly what text editor or IDE is used for development is up to developer preference, but there are several tools that have been used in initial development of _KoMo2_ to increase code readability and quality:

- **ClangFormat** is a C and C++ formatter that can be run on a source program to create a consistent, human-readable file, and leaves you not having to worry about styling. More information can be found [here](https://clang.llvm.org/docs/ClangFormat.html).

  This program has been developed using the default settings for ClangFormat, however you can configure a custom formatter file if you desire.

* **Doxygen** is a tool that allows for generation of documentation on classes, variables and functions from specially styled comments embedded in the code. Many doxygen comments can be found in the code base, but an example of a basic Doxygen comment is as follows:

  ```c++
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

  It is recommended that any new files, classes, functions or member variables that are added to the codebase have a full Doxygen comment provided with them. No specific rules about how to write these comments are in place, but ensure they are clear on what the file, class, function or member variable is for.

#### Areas for improvement

The motivation for the dissertation project which spawned the _KoMo2_ was as follows:

###### "._..there is a need for an updated and improved Jimulator GUI which is more up to date, better designed, more maintainable, and more accessible to a diverse set of users._"

While this project does represent an improvement on _KoMoDo_, there are still some features that could improve within _KoMo2_ that were outside the scope of this project.

Some possible areas for improvement are as follows:

1. The source code for the `bin/aasm` binary, (found in `src/aasmSrc/`) is a mess. It could do with some serious revisions or at least some documentation.
2. The source code for the `bin/jimulator` binary (found in `src/jimulatorSrc/`) is a mess. It could do with some serious revisions or at least some documentation.
3. Currently, the `aasm` and `jimulator` binaries are forked from the program as separate processes, and communicated with through pipes. This presents some issues, such as a particularly old school piping API that primarily works through messages sent as binary representations of data.
   Moving the source for these programs into the Jimulator executable and launching them as separate threads would allow for far clearer and easier communication between _KoMo2_ and it's child modules.
   This would also have the additional benefit of (potentially) allowing for cross platform compilation of the program onto Windows and MacOS.
4. The graphics library _GTKMM_ is going to be receiving it's 4.0 revision in the coming months (as of April 2021). For future proofing, it may be worth experimenting with and implementing any new features that _GTKMM-4_ provides, and ensuring that _KoMo2_ does not rely on any features being deprecated by this revision.
5. Web technologies, such as Javascript, CSS, and HTML, are becoming the norm for GUI programming. There are ways to interface C++ with these web technologies which could allow for a more dynamic and agile GUI development experience.

#### Submitting your contributions

If you do make any improvements or additions to _KoMo2_, please open a pull request on the GitHub repository. I will happily discuss and review your changes, and merge them into the trunk if they are suitable. 🎉

---
