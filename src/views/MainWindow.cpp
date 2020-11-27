#include "MainWindow.h"

MainWindow::MainWindow() : fileSelector("Hello World") {
  set_border_width(10);
  set_default_size(1150, 725);
  add(fileSelector);
  fileSelector.show();
}

MainWindow::~MainWindow() {}
