/**
 * @file main.cpp
 * @author Lawrence Warren (lawrencewarren2@gmail.com)
 * @brief The entry point of the program. Should have as little logic as
 * possible.
 * @version 1.0.0
 * @date 10-04-2021
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

#include <glib.h>
#include <glibmm/optioncontext.h>
#include <gtkmm/application.h>
#include <libgen.h>
#include <sys/signal.h>
#include <unistd.h>
#include <array>
#include <fstream>
#include <iostream>
#include <regex>
#include <sstream>
#include <utility>
#include <vector>
#include "../../libs/rapidjson/document.h"
#include "models/KoMo2Model.h"
#include "views/MainWindowView.h"

int writeToJimulator;
int readFromJimulator;
int emulator_PID = -1;

// Communication pipes
// ! Originally board_emulation_communication_from
int communicationFromJimulator[2];
// ! Originally board_emulation_communication_to
int communicationToJimulator[2];
// ! Originally compile_communication
int compilerCommunication[2];

// Default values for variables read from `variables.json`
std::string version = "1.0.0";
std::string manual = "https://github.com/LawrenceWarren/KoMo2#user-manual";
std::string help = "Please view the user manual (" + manual + ") for help.";
int refresh = 200;

void initJimulator(const std::string argv0);
void initCompilerPipes(KoMo2Model* const mainModel);
const std::string getAbsolutePathToRootDirectory(const char* const arg);
const int initialiseCommandLine(
    const Glib::RefPtr<Gio::ApplicationCommandLine>&,
    const Glib::RefPtr<Gtk::Application>& app);
const bool receivedCompilerOutput(GIOChannel* source,
                                  GIOCondition condition,
                                  gpointer data);
void readProgramVariables(const std::string argv0);

// A global reference to the mainModel
void* model;

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

  readProgramVariables(argv0);

  // Setup communication to Jimulator child process
  initJimulator(argv0);

  // Setup command line argument recognition
  app->signal_command_line().connect(
      sigc::bind(sigc::ptr_fun(initialiseCommandLine), app), false);

  // Setup model & view
  MainWindowView koMo2Window(400, 400);
  KoMo2Model mainModel(&koMo2Window, argv0, manual, refresh);

  model = &mainModel;

  // Setup communication methods to compile child process
  initCompilerPipes(&mainModel);

  // Run
  auto exit = app->run(koMo2Window);
  kill(emulator_PID, SIGKILL);  // Kill Jimulator if main window is killed
  return exit;
}

/**
 * @brief Initialises the communication pipes between the compiler process & the
 * main KoMo2 GUI.
 * @param mainModel A pointer to the main KoMo2 model.
 */
void initCompilerPipes(KoMo2Model* mainModel) {
  // Allows for printing compile output in the KoMo2 terminal
  if (pipe(compilerCommunication)) {
    std::cout << "A pipe error ocurred." << std::endl;
    exit(1);
  }

  // Sets up the function `receivedCompilerOutput` to fire whenever a change to
  // the `compilerCommunication` pipe occurs. This in turn calls a function
  // in the mainModel object.
  auto fd = g_io_channel_unix_new(compilerCommunication[0]);
  auto f = (GIOFunc)receivedCompilerOutput;
  g_io_add_watch(fd, G_IO_IN, f, mainModel);
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
  // sets up the pipes to allow communication between Jimulator and
  // KoMo2 processes.
  if (pipe(communicationFromJimulator) || pipe(communicationToJimulator)) {
    std::cout << "A pipe error ocurred." << std::endl;
    exit(1);
  }

  readFromJimulator = communicationFromJimulator[0];
  writeToJimulator = communicationToJimulator[1];

  // Stores the emulator_PID for later.
  emulator_PID = fork();

  // Jimulator process.
  if (emulator_PID == 0) {
    // Closes Jimulator stdout - Jimulator can write to this pipe using printf
    close(1);
    dup2(communicationFromJimulator[1], 1);

    // Closes Jimulator stdin - Jimulator can write to this pipe using scanf
    close(0);
    dup2(communicationToJimulator[0], 0);

    auto jimulatorPath = argv0.append("/bin/jimulator").c_str();
    execlp(jimulatorPath, "", (char*)0);
    // should never get here
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
 * @brief Reads the program variables from the "variables.json" file and
 * populates global variables with the values read.
 */
void readProgramVariables(const std::string argv0) {
  // Reading the file into a large string
  std::string s;
  std::ifstream variablesFile(argv0 + "/variables.json");
  std::stringstream ss;
  bool first = true;

  if (variablesFile.is_open()) {
    while (getline(variablesFile, s)) {
      if (first) {
        first = false;
      } else {
        ss << '\n';
      }

      ss << s;
    }
    variablesFile.close();
  }

  // JSON parsing the string
  rapidjson::Document d;
  d.Parse(ss.str().c_str());

  if (not d.IsObject()) {
    std::cout << "Can't read variables.json, using defaults." << std::endl;
    return;
  }

  if (d.HasMember("version")) {
    version = std::string(d["version"].GetString());
  }

  if (d.HasMember("manual")) {
    manual = std::string(d["manual"].GetString());
  }

  if (d.HasMember("help")) {
    help = std::string(d["help"].GetString());
  } else {
    help = "Please view the user manual (" + manual + ") for help.";
  }

  if (d.HasMember("refresh")) {
    refresh = d["refresh"].GetInt();
  }
}

/**
 * @brief Handles if a command line flag is set.
 * @param isVersion If the version flag has been set.
 * @param isHelp If the help flag has been set.
 * @param isMnemonics If the mnemonics flag is set.
 * @return int status code.
 */
const int handleCommandLine(const bool isVersion,
                            const bool isHelp,
                            const bool isMnemonics) {
  if (isMnemonics) {
    static_cast<KoMo2Model*>(model)->getDisassemblyModel()->setEnglishMnemonic(
        true);
  }

  if (isVersion) {
    std::cout << version << std::endl;
    return 1;
  }

  if (isHelp) {
    std::cout << help << std::endl;
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
  bool isVersion = false, isHelp = false,
       isMnemonics = false;  // Booleans for options being toggled
  Glib::OptionEntry version, help, mnemonics;  // Option objects

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

  mnemonics.set_long_name("english");
  mnemonics.set_short_name('e');
  mnemonics.set_description(
      "Play ARM mnemonics in English if using a screenreader.");
  group.add_entry(mnemonics, isMnemonics);

  Glib::OptionContext context;
  context.add_group(group);

  // Parses command line arguments.
  Glib::OptionGroup gtkgroup(gtk_get_option_group(true));
  context.add_group(gtkgroup);
  int argc;
  char** argv = cmd->get_arguments(argc);
  context.parse(argc, argv);

  int returnVal = handleCommandLine(isVersion, isHelp, isMnemonics);

  // If command line arguments were valid, activate app, else not.
  if (not returnVal) {
    app->activate();
  }

  return returnVal;
}

/**
 * @brief This callback function is called when the compiler communication pipe
 * receives a change in state (meaning the compiler process has sent some data).
 * This function will then send that data to the integrated terminal within
 * KoMo2.
 * @param source A pointer to the file descriptor which had a change of state
 * and caused this callback function to fire.
 * @param condition The type of state that changed to cause this callback to
 * fire - for example, new data is to be read from the file descriptor, or data
 * can be written to the file descriptor.
 * @param data Any additional data - in this case, a pointer to the KoMo2 model
 * is sent.
 * @return true There is more to be handled - this function can be called again
 * to handle a related state change.
 * @return false The function is done handling the state change.
 */
const bool receivedCompilerOutput(GIOChannel* source,
                                  GIOCondition condition,
                                  gpointer data) {
  // data is always a pointer to the main window
  auto p = static_cast<KoMo2Model*>(data);
  char buff[128];  // An arbitrary size

  // The amount of bytes that were read
  int x = read(compilerCommunication[0], buff, 128);

  // There was a pipe reading failure.
  if (x < 0) {
    // TODO: handle error gracefully
    std::cout << "Error reading from compiler communcation pipe." << std::endl;
    return false;
  }
  // There is no more data to be read.
  else if (x == 0) {
    return false;
  }

  buff[x] = '\0';
  p->getTerminalModel()->appendTextToTextView(buff);
  return true;
}
