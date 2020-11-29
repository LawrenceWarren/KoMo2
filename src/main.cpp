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
#include <sys/signal.h>
#include <unistd.h>
#include <iostream>
#include "globals.h"
#include "views/MainWindow.h"

source_file kmdSourceFile;
target_system* board;
symbol* symbol_table;
int symbol_count;
int writeToJimulator;
int readFromJimulator;
int emulator_PID = -1;

int board_emulation_communication_from[2];
int board_emulation_communication_to[2];

void initJimulator(std::string argv0);
std::string getAbsolutePathToBinaryDirectory(char* arg);
bool openDotKomodo(std::string pathToBinaryDir);

/**
 * @brief The program entry point.
 * @param argc The number of command line arguments.
 * @param argv An array of command line arguments.
 * @return int exit code.
 */
int main(int argc, char* argv[]) {
  std::string argv0 = getAbsolutePathToBinaryDirectory(argv[0]);
  openDotKomodo(argv0);
  initJimulator(argv0);

  auto app = Gtk::Application::create(argc, argv, "uon.komodo");
  MainWindow KoMo2(argv0);
  auto exit = app->run(KoMo2);

  kill(emulator_PID, SIGKILL);  // Kill Jimulator
  return exit;
}

/**
 * @brief Opens `bin/dotkomodo.string` file.
 * @return auto exit code.
 */
bool openDotKomodo(std::string pathToBinaryDir) {
  PtrSCANNode scantopnode = nullptr;
  GScanner* scanner = nullptr;
  int use_internal = 0;

  // TODO: read `bin/dotkomodo.string`
  // If internal file is used
  if (use_internal) {
    // scanner = ScanOpenSCANString(dotkomodo);
  } else {
    // scanner = ScanOpenSCANFile(rcfile);
  }

  // Check the state of file scanner
  if (scanner) {
    std::cout
        << "Cannot find set up file `.komodo'" << std::endl
        << "If you would like to create one then start komodo with -c flag"
        << std::endl;
    exit(1);
  }

  // TODO: call these functions
  // scantopnode = ScanParseSCANNode(scanner, FALSE);  // Scan .komodo file
  // ScanCloseSCANFile(scanner);  // The scanner can safely be closed now

  // Check if the parse was successful
  if (scantopnode) {
    return true;  // Success
  } else {
    return false;  // failure
  }
}

/**
 * @brief Initialises Jimulator in a separate child process.
 * @param argv0 A path to the directory where the KoMo2 executable exists. It is
 * assumed that all other files required by the program at run time (executables
 * and resources) live in the same directory.
 * For example, if the KoMo2 executable exists at `/home/user/demo/kmd` then
 * argv0 will have the value `/home/user/demo` and it will be assumed that the
 * Jimulator executable lives at `/home/user/demo/jimulator`.
 * @return auto.
 */
void initJimulator(std::string argv0) {
  // Clears the symbol tables
  symbol_table = nullptr;
  symbol_count = 0;

  // sets up the pipes to allow communication between Jimulator and
  // KoMo2 processes.
  if (pipe(board_emulation_communication_from) ||
      pipe(board_emulation_communication_to)) {
    std::cout << "A pipe error ocurred." << std::endl;
    exit(1);
  }

  readFromJimulator = board_emulation_communication_from[0];
  writeToJimulator = board_emulation_communication_to[1];

  // Stores the emulator_PID for later.
  emulator_PID = fork();

  // Jimulator process.
  if (emulator_PID == 0) {
    // Closes Jimulator stdout - Jimulator can write to this pipe using printf
    close(1);
    dup2(board_emulation_communication_from[1], 1);

    // Closes Jimulator stdin - Jimulator can write to this pipe using scanf
    close(0);
    dup2(board_emulation_communication_to[0], 0);

    const char* jimulatorPath = argv0.append("/jimulator").c_str();
    execlp(jimulatorPath, "", (char*)0);
    std::cout << "Jimulator launching has failed" << std::endl;
    _exit(1);
  }
}

/**
 * @brief get the absolute path to the directory of the binary.
 * @return auto the directory of the KoMo2 binary - if the binary is at
 * `/home/user/demo/kmd`, return `/home/user/demo`.
 */
std::string getAbsolutePathToBinaryDirectory(char* arg) {
  // Gets a path to this executable
  char* buf = realpath(arg, NULL);
  std::string argv0(buf);
  free(buf);

  argv0 = argv0.substr(0, argv0.size() - 4);
  return argv0;
}
