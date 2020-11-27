/**
 * @file MainWindow.cpp
 * @author Lawrence Warren (lawrencewarren2@gmail.com)
 * @brief The definition of the class `MainWindow` and all of it's related functions
 * and members.
 * @version 0.1
 * @date 2020-11-27
 *
 * @section LICENSE
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 3 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details at
 * https://www.gnu.org/copyleft/gpl.html
 *
 * @copyright Copyright (c) 2020
 *
 */

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
