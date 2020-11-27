#include "MainWindow.h"

MainWindow::MainWindow()
    : selectAndLoadContainer(),
      compileAndLoad("Compile & Load"),
      fileSelector("Select File", &compileAndLoad) {
  set_border_width(10);
  set_default_size(1150, 725);

  selectAndLoadContainer.pack_end(fileSelector);
  selectAndLoadContainer.pack_end(compileAndLoad);
  selectAndLoadContainer.show_all_children();

  add(selectAndLoadContainer);
  selectAndLoadContainer.show();
}

MainWindow::~MainWindow() {}
