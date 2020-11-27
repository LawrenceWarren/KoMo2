/**
 * @file main.cpp
 * @author Lawrence Warren (lawrencewarren2@gmail.com)
 * @brief The entry point of the program. Should have as little logic as
 * possible.
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
#include <gtkmm/application.h>
#include "views/MainWindow.h"

int main(int argc, char* argv[]) {
  auto app = Gtk::Application::create(argc, argv, "uon.komodo");
  MainWindow KoMo2;
  return app->run(KoMo2);
}
