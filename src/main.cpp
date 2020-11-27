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
#include <libgen.h>
#include <unistd.h>
#include <iostream>
#include "views/MainWindow.h"

int main(int argc, char* argv[]) {
  // Gets a path to this executable
  char buf[255];
  if (readlink("/proc/self/exe", buf, sizeof(buf)) == -1) {
    std::cout << "could not find executable path!" << std::endl;
    exit(1);
  }

  // Turns the path into a std string, removes "/kmd"
  std::string argv0(buf);
  argv0 = argv0.substr(0, argv0.size() - 4);

  // Launch the app
  auto app = Gtk::Application::create(argc, argv, "uon.komodo");
  MainWindow KoMo2(argv0);
  return app->run(KoMo2);
}
