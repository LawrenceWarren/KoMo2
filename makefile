# Do all
all: kmd aasm jimulator

# Compile the KoMo2 binary.
# ! WARNING ! Compiling with -O3 (highest level optimisations) is considered
# ! dangerous, the optimisations could introduce bugs. However it seems to work
# ! as of 24/12/2020.
# ! If any bugs are found, please attempt to compile with -O2 or -O1 and
# ! recreate the bug, before assuming fault of the program
kmd: src/kmdSrc/views/CompileLoadView.cpp src/kmdSrc/views/ControlsView.cpp src/kmdSrc/models/ControlsModel.cpp src/kmdSrc/compile.c src/kmdSrc/views/MainWindowView.cpp src/kmdSrc/models/Model.cpp src/kmdSrc/models/CompileLoadModel.cpp src/kmdSrc/models/KoMo2Model.cpp src/kmdSrc/main.cpp 
	g++ `pkg-config --cflags gtkmm-3.0` -o bin/kmd src/kmdSrc/views/CompileLoadView.cpp src/kmdSrc/views/ControlsView.cpp src/kmdSrc/compile.c src/kmdSrc/models/KoMo2Model.cpp  src/kmdSrc/models/CompileLoadModel.cpp src/kmdSrc/models/Model.cpp  src/kmdSrc/models/ControlsModel.cpp src/kmdSrc/views/MainWindowView.cpp src/kmdSrc/main.cpp `pkg-config --libs gtkmm-3.0` -Wall -O3

# Compile the arm assember binary.
aasm: src/aasmSrc/aasm.c
	gcc -O0 -o bin/aasm src/aasmSrc/aasm.c

# Compile the jimulator binary.
jimulator: src/jimulatorSrc/jimulator.c
	gcc `pkg-config --cflags gtkmm-3.0` -o bin/jimulator src/jimulatorSrc/jimulator.c `pkg-config --libs gtkmm-3.0` -O2
