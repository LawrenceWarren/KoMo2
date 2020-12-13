all: kmd aasm jimulator #appendKmd

kmd: src/compile.c src/views/MainWindow.cpp src/views/CompileLoadButton.cpp src/views/BrowseButton.cpp src/main.cpp 
	g++ `pkg-config --cflags gtkmm-3.0` -o bin/kmd src/compile.c src/views/BrowseButton.cpp src/views/CompileLoadButton.cpp src/views/MainWindow.cpp src/main.cpp `pkg-config --libs gtkmm-3.0` -Wall -O2

aasm: src/aasmSrc/aasm.c
	gcc -O0 -o bin/aasm src/aasmSrc/aasm.c -Wall

jimulator: src/jimulatorSrc/jimulator.c
	gcc `pkg-config --cflags gtkmm-3.0` -o bin/jimulator src/jimulatorSrc/jimulator.c `pkg-config --libs gtkmm-3.0` -O2 -Wall

# appendKmd: bin/kmd
# 	readlink -f bin/kmd 