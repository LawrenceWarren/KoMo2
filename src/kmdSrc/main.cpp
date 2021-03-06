/**
 * @file main.cpp
 * @author Lawrence Warren (lawrencewarren2@gmail.com)
 * @brief The entry point of the program. Should have as little logic as
 * possible.
 * @version 0.1
 * @date 2020-12-23
 * @section LICENSE
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 3 of
 * the License, or (at your option) any later version.
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details at
 * https://www.gnu.org/copyleft/gpl.html

 */

#include <glibmm/optioncontext.h>
#include <gtkmm/application.h>
#include <libgen.h>
#include <sys/signal.h>
#include <unistd.h>
#include <fstream>
#include <iostream>
#include "globals.h"
#include "models/KoMo2Model.h"
#include "views/MainWindowView.h"

// Init global variables
symbol* symbol_table;
int symbol_count;
int writeToJimulator;
int readFromJimulator;
int emulator_PID = -1;

// Communication pipes
// ! Originally board_emulation_communication_from
int communicationFromPipe[2];
// ! Originally board_emulation_communication_to
int communicationToPipe[2];

void initJimulator(std::string argv0);
const std::string getAbsolutePathToRootDirectory(const char* const arg);
const int initialiseCommandLine(
    const Glib::RefPtr<Gio::ApplicationCommandLine>&,
    const Glib::RefPtr<Gtk::Application>& app);

/**
 * @brief The program entry point.
 * @param argc The number of command line arguments.
 * @param argv An array of command line arguments.
 * @return int exit code.
 */
int main(int argc, char* argv[]) {
  auto argv0 = getAbsolutePathToRootDirectory(argv[0]);

  auto app = Gtk::Application::create(argc, argv, "uon.cs.KoMo2",
                                      Gio::APPLICATION_HANDLES_COMMAND_LINE);
  app->signal_command_line().connect(
      sigc::bind(sigc::ptr_fun(initialiseCommandLine), app), false);

  initJimulator(argv0);  // Creates Jimulator

  MainWindowView koMo2Window(1240, 700);
  KoMo2Model mainModel(&koMo2Window, argv0);

  auto exit = app->run(koMo2Window);
  kill(emulator_PID, SIGKILL);  // Kill Jimulator
  return exit;
}

/**
 * @brief Initialises Jimulator in a separate child process.
 * @param argv0 A path to the directory where the KoMo2 executable exists. It is
 * assumed that all other files required by the program at run time (executables
 * and resources) live in the same directory.
 * For example, if the KoMo2 executable exists at `/home/user/demo/kmd` then
 * argv0 will have the value `/home/user/demo` and it will be assumed that the
 * Jimulator executable lives at `/home/user/demo/jimulator`.
 */
void initJimulator(std::string argv0) {
  // Clears the symbol tables
  symbol_table = nullptr;
  symbol_count = 0;

  // sets up the pipes to allow communication between Jimulator and
  // KoMo2 processes.
  if (pipe(communicationFromPipe) || pipe(communicationToPipe)) {
    std::cout << "A pipe error ocurred." << std::endl;
    exit(1);
  }

  readFromJimulator = communicationFromPipe[0];
  writeToJimulator = communicationToPipe[1];

  // Stores the emulator_PID for later.
  emulator_PID = fork();

  // Jimulator process.
  if (emulator_PID == 0) {
    // Closes Jimulator stdout - Jimulator can write to this pipe using printf
    close(1);
    dup2(communicationFromPipe[1], 1);

    // Closes Jimulator stdin - Jimulator can write to this pipe using scanf
    close(0);
    dup2(communicationToPipe[0], 0);

    auto jimulatorPath = argv0.append("/bin/jimulator").c_str();
    execlp(jimulatorPath, "", (char*)0);
    std::cout << "Jimulator launching has failed" << std::endl;
    _exit(1);
  }
}

/**
 * @brief get the absolute path to the directory of the binary.
 * @return std::string the directory of the KoMo2 binary - if the binary is at
 * `/home/user/demo/kmd`, return `/home/user/demo`.
 */
const std::string getAbsolutePathToRootDirectory(const char* const arg) {
  // Gets a path to this executable
  auto buf = realpath(arg, NULL);
  const std::string argv0(buf);
  free(buf);

  return argv0.substr(0, argv0.size() - 7);
}

/**
 * @brief Returns the version number as a string.
 * @returns std::string the version number contained in the plain text file
 * `version` in the project root.
 */
const std::string getVersionNumber() {
  std::string versionString;
  std::ifstream versionFile("version");

  if (versionFile.is_open()) {
    if (not std::getline(versionFile, versionString)) {
      std::cout << "error!";
    }
    versionFile.close();
  }

  return versionString;
}

/**
 * @brief read and print the help message.
 */
void printHelpMessage() {
  std::cout << "TODO: Help message!" << std::endl;
  // TODO: stub - read a help message and print it from somewhere
}

/**
 * @brief Handles if a command line flag is set.
 * @param isVersion If the version flag has been set.
 * @param isHelp If the help flag has been set.
 * @return int status code.
 */
const int handleCommandLine(const bool isVersion, const bool isHelp) {
  // TODO: potentially move version & help message to JSON?
  if (isVersion) {
    std::cout << "v" << getVersionNumber() << std::endl;
    return 1;
  }

  if (isHelp) {
    printHelpMessage();
    return 1;
  }

  return 0;
}

/**
 * @brief Sets up and parses custom command line arguments.
 * To add more command line arguments, create a new boolean and a new
 * Glib::OptionEntry object, and then add a longname, shortname, and
 * description, then add it to the group. Then pass the boolean into the
 * handleCommandLine function.
 * @param cmd The apps command line object.
 * @param app The GTK application itself.
 * @return int status code.
 */
const int initialiseCommandLine(
    const Glib::RefPtr<Gio::ApplicationCommandLine>& cmd,
    const Glib::RefPtr<Gtk::Application>& app) {
  Glib::OptionGroup group("options", "main options");
  bool isVersion = false,
       isHelp = false;              // Booleans for options being toggled
  Glib::OptionEntry version, help;  // Option objects

  // Setting the version `-v` option
  version.set_long_name("version");
  version.set_short_name('v');
  version.set_description("Display version information.");
  group.add_entry(version, isVersion);

  // Setting the help `-h` option
  help.set_long_name("help");
  help.set_short_name('h');
  help.set_description("Display a help message.");
  group.add_entry(help, isHelp);

  Glib::OptionContext context;
  context.add_group(group);

  // Parses command line arguments.
  Glib::OptionGroup gtkgroup(gtk_get_option_group(true));
  context.add_group(gtkgroup);
  int argc;
  char** argv = cmd->get_arguments(argc);
  context.parse(argc, argv);

  int returnVal = handleCommandLine(isVersion, isHelp);

  // If command line arguments were valid, activate app, else not.
  if (not returnVal) {
    app->activate();
  }

  return returnVal;
}
