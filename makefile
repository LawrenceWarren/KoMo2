all: kmd aasm jimulator

kmd: src/kmdSrc/models/ControlsModel.cpp src/kmdSrc/compile.c src/kmdSrc/views/MainWindow.cpp src/kmdSrc/models/CompileLoadModel.cpp src/kmdSrc/models/KoMo2Model.cpp src/kmdSrc/main.cpp 
	g++ `pkg-config --cflags gtkmm-3.0` -o bin/kmd src/kmdSrc/compile.c src/kmdSrc/models/KoMo2Model.cpp  src/kmdSrc/models/CompileLoadModel.cpp  src/kmdSrc/models/ControlsModel.cpp src/kmdSrc/views/MainWindow.cpp src/kmdSrc/main.cpp `pkg-config --libs gtkmm-3.0` -Wall -O2

aasm: src/aasmSrc/aasm.c
	gcc -O0 -o bin/aasm src/aasmSrc/aasm.c

jimulator: src/jimulatorSrc/jimulator.c
	gcc `pkg-config --cflags gtkmm-3.0` -o bin/jimulator src/jimulatorSrc/jimulator.c `pkg-config --libs gtkmm-3.0` -O2
