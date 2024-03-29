# Do all
all: jimulator kmd

# Compile the KoMo2 binary.
# ! WARNING ! Compiling with -O3 (highest level optimisations) is considered
# ! dangerous, the optimisations could introduce bugs. Works as of
# ! 10/04/2021
# ! If any bugs are found, please attempt to compile with -O2 or -O1 and
# ! recreate the bug, before assuming fault of the program
kmd: src/kmdSrc/views/TerminalView.cpp src/kmdSrc/views/DisassemblyView.cpp src/kmdSrc/models/DisassemblyModel.cpp src/kmdSrc/models/TerminalModel.cpp src/kmdSrc/views/RegistersView.cpp src/kmdSrc/models/RegistersModel.cpp src/kmdSrc/views/CompileLoadView.cpp src/kmdSrc/views/ControlsView.cpp src/kmdSrc/models/ControlsModel.cpp src/kmdSrc/jimulatorInterface.cpp src/kmdSrc/views/MainWindowView.cpp src/kmdSrc/models/Model.cpp src/kmdSrc/models/CompileLoadModel.cpp src/kmdSrc/models/KoMo2Model.cpp src/kmdSrc/main.cpp 
	g++ `pkg-config --cflags gtkmm-3.0` -o bin/kmd src/kmdSrc/views/RegistersView.cpp src/kmdSrc/models/RegistersModel.cpp src/kmdSrc/views/CompileLoadView.cpp src/kmdSrc/views/ControlsView.cpp src/kmdSrc/jimulatorInterface.cpp src/kmdSrc/models/KoMo2Model.cpp  src/kmdSrc/models/CompileLoadModel.cpp src/kmdSrc/models/Model.cpp  src/kmdSrc/models/ControlsModel.cpp src/kmdSrc/views/MainWindowView.cpp src/kmdSrc/views/TerminalView.cpp src/kmdSrc/views/DisassemblyView.cpp src/kmdSrc/models/DisassemblyModel.cpp src/kmdSrc/models/TerminalModel.cpp src/kmdSrc/main.cpp `pkg-config --libs gtkmm-3.0` -Wall -Wextra -O3 -std=c++17

# Compile the arm assember binary.
aasm: src/aasmSrc/aasm.cpp
	g++ -O0 -o bin/aasm src/aasmSrc/aasm.cpp -Wall -Wextra

kcmd: src/kcmdSrc/kcmd.cpp
	g++ src/kcmdSrc/kcmd.cpp -o bin/kcmd -std=c++17 -pthread

# Compile the jimulator binary.
jimulator: src/jimulatorSrc/jimulator.cpp
	g++ -o bin/jimulator src/jimulatorSrc/jimulator.cpp -Wall -Wextra -O3 -std=c++17
