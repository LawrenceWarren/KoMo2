# Do all
all: kmd aasm jimulator

# Compile the KoMo2 binary.
# ! WARNING ! Compiling with -O3 (highest level optimisations) is considered
# ! dangerous, the optimisations could introduce bugs. Does not work 
# ! as of 23/01/2021. 
# ! If any bugs are found, please attempt to compile with -O2 or -O1 and
# ! recreate the bug, before assuming fault of the program
kmd: src/kmdSrc/views/TerminalView.cpp src/kmdSrc/models/TerminalModel.cpp src/kmdSrc/views/RegistersView.cpp src/kmdSrc/models/RegistersModel.cpp src/kmdSrc/views/CompileLoadView.cpp src/kmdSrc/views/ControlsView.cpp src/kmdSrc/models/ControlsModel.cpp src/kmdSrc/jimulatorInterface.cpp src/kmdSrc/views/MainWindowView.cpp src/kmdSrc/models/Model.cpp src/kmdSrc/models/CompileLoadModel.cpp src/kmdSrc/models/KoMo2Model.cpp src/kmdSrc/main.cpp 
	g++ `pkg-config --cflags gtkmm-3.0` -o bin/kmd src/kmdSrc/views/RegistersView.cpp src/kmdSrc/models/RegistersModel.cpp src/kmdSrc/views/CompileLoadView.cpp src/kmdSrc/views/ControlsView.cpp src/kmdSrc/jimulatorInterface.cpp src/kmdSrc/models/KoMo2Model.cpp  src/kmdSrc/models/CompileLoadModel.cpp src/kmdSrc/models/Model.cpp  src/kmdSrc/models/ControlsModel.cpp src/kmdSrc/views/MainWindowView.cpp src/kmdSrc/views/TerminalView.cpp src/kmdSrc/models/TerminalModel.cpp src/kmdSrc/main.cpp `pkg-config --libs gtkmm-3.0` -Wall -O2

# Compile the arm assember binary.
aasm: src/aasmSrc/aasm.c
	gcc -O0 -o bin/aasm src/aasmSrc/aasm.c

# Compile the jimulator binary.
jimulator: src/jimulatorSrc/jimulator.c
	gcc `pkg-config --cflags gtkmm-3.0` -o bin/jimulator src/jimulatorSrc/jimulator.c `pkg-config --libs gtkmm-3.0` -O2
