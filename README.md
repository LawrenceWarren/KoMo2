# KoMo2:

## Modernising an Advanced Reduced Instruction Set Computing Machine architecture emulator

###### _A 3^rd^ year dissertation project by Lawrence Warren for Doctor Steven Bagley PhD. of the University of Nottingham._

### Dependencies

_KoMo2_ is implemented using _GTKMM_, a C++ implementation of the popular GUI library _GTK-3_. This in turn has many of it's own dependencies - a full list can be found [here](https://developer.gnome.org/gtkmm-tutorial/stable/sec-installation-dependencies.html.en). 

These libraries are popular and can likely be found on your Linux distributions package manager. However, it is possible to compile them from source - instructions are readily available online.

### Assumptions

This program relies on the executable `bin/kmd_compile` - it is assumed that this executable will be in this directory, and the executable is tracked in the source repository. Furthermore, `bin/kmd_compile` does not have source code, so any assumptions it makes, we must follow.

`bin/kmd_compile` also relies on several other files:

- `aasm`, another executable with missing source code - `bin/kmd_compile` assumes `aasm` is located in `_____________`.
- `aasm` in turn relies on a plain text file, `mnemonics`, which must be in the same directory.
