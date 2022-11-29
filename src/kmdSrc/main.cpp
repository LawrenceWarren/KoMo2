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
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * See the GNU General Public License for more details at
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
#include <map>
#include <regex>
#include <sstream>
#include <utility>
#include <vector>
#include "models/KoMo2Model.h"
#include "views/MainWindowView.h"

// Forward function declarations
const int init_jimulator(const std::string);
void init_compiler_pipes(KoMo2Model* const);
const std::string get_absolute_path_to_root_dir(const char* const);
const int init_cli(const Glib::RefPtr<Gio::ApplicationCommandLine>&,
                   const Glib::RefPtr<Gtk::Application>&,
                   std::string,
                   std::string,
                   KoMo2Model*);
const bool receive_compiler_output(GIOChannel*, GIOCondition, gpointer);
std::map<std::string, std::string> read_program_config(const std::string);

// Communication pipes, Defined as extern in jimulatorInterface.h
int comms_from_jimulator[2];
int comms_to_jimulator[2];
int compiler_comms[2];
int write_jimulator;
int read_jimulator;

/**
 * @brief The program entry point.
 * @param argc The number of command line arguments.
 * @param argv An array of command line arguments.
 * @return int exit code.
 */
int main(int argc, char* argv[]) {
  auto argv0 = get_absolute_path_to_root_dir(argv[0]);
  auto cm = read_program_config(argv0);
  auto jimulator_pid = init_jimulator(argv0);
  KoMo2Model* p_model;
  auto app = Gtk::Application::create(argc, argv, "uon.cs.KoMo2",
                                      Gio::APPLICATION_HANDLES_COMMAND_LINE);
  auto ptr_func = sigc::ptr_fun(init_cli);
  auto bound = sigc::bind(ptr_func, app, cm["version"], cm["help"], p_model);
  app->signal_command_line().connect(bound, false);
  MainWindowView km2_window(400, 400);
  KoMo2Model model(&km2_window, argv0, cm["manual"], std::stoi(cm["refresh"]));
  p_model = &model;
  init_compiler_pipes(&model);

  // Run
  auto exit = app->run(km2_window);
  kill(jimulator_pid, SIGKILL);  // Kill Jimulator if main window is killed
  return exit;
}

/**
 * @brief Initialises the communication pipes between the compiler process & the
 * main KoMo2 GUI.
 * @param model A pointer to the main KoMo2 model.
 */
void init_compiler_pipes(KoMo2Model* model) {
  // Allows for printing compile output in the KoMo2 terminal
  if (pipe(compiler_comms)) {
    std::cout << "A pipe error ocurred." << std::endl;
    exit(1);
  }

  // Sets up the function `receive_compiler_output` to fire whenever a change to
  // the `compiler_comms` pipe occurs. This in turn calls a function
  // in the model object.
  auto fd = g_io_channel_unix_new(compiler_comms[0]);
  auto f = (GIOFunc)receive_compiler_output;
  g_io_add_watch(fd, G_IO_IN, f, model);
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
const int init_jimulator(std::string argv0) {
  // sets up the pipes to allow communication between Jimulator and
  // KoMo2 processes.
  if (pipe(comms_from_jimulator) || pipe(comms_to_jimulator)) {
    std::cout << "A pipe error ocurred." << std::endl;
    exit(1);
  }

  read_jimulator = comms_from_jimulator[0];
  write_jimulator = comms_to_jimulator[1];

  auto jimulator_pid = fork();

  // Jimulator process.
  if (jimulator_pid == 0) {
    // Closes Jimulator stdout - Jimulator can write to this pipe using printf
    close(1);
    dup2(comms_from_jimulator[1], 1);

    // Closes Jimulator stdin - Jimulator can write to this pipe using scanf
    close(0);
    dup2(comms_to_jimulator[0], 0);

    auto jimulator_path = argv0.append("/bin/jimulator").c_str();
    execlp(jimulator_path, "", (char*)0);
    // should never get here
    _exit(1);
  }
  return jimulator_pid;
}

/**
 * @brief get the absolute path to the directory of the binary.
 * @return std::string the directory of the KoMo2 binary - if the binary is at
 * `/home/user/demo/kmd`, return `/home/user/demo`.
 */
const std::string get_absolute_path_to_root_dir(const char* const arg) {
  // Gets a path to this executable
  auto buf = realpath(arg, NULL);
  const std::string argv0(buf);
  free(buf);

  return argv0.substr(0, argv0.size() - 7);
}

/**
 * @brief Reads the program variables from the "config.rc" file and populates
 * the global variables with the values read.
 */
std::map<std::string, std::string> read_program_config(
    const std::string argv0) {
  // Reading the file into a large string
  std::string key, value;
  std::ifstream config_file(argv0 + "/config.rc");
  std::map<std::string, std::string> cm;

  while (config_file.is_open() and std::getline(config_file, key, '=')) {
    std::getline(config_file, value);
    cm.insert_or_assign(key, value);
  }

  config_file.close();
  return cm;
}

/**
 * @brief Handles if a command line flag is set.
 * @param is_version If the version flag has been set.
 * @param is_help If the help flag has been set.
 * @param is_mnemonic If the mnemonics flag is set.
 * @param version The version number.
 * @param help The help message.
 * @return int status code.
 */
const int handle_cli(const bool is_version,
                     const bool is_help,
                     const bool is_mnemonic,
                     const std::string version,
                     const std::string help,
                     KoMo2Model* model) {
  if (is_mnemonic) {
    model->getDisassemblyModel()->setEnglishMnemonic(true);
  }

  if (is_version) {
    std::cout << version << std::endl;
    exit(1);
  }

  if (is_help) {
    std::cout << help << std::endl;
    exit(1);
  }

  return 0;
}

/**
 * @brief Sets up and parses custom command line arguments.
 * To add more command line arguments, create a new boolean and a new
 * Glib::OptionEntry object, and then add a longname, shortname, and
 * description, then add it to the group. Then pass the boolean into the
 * handle_cli function.
 * @param cmd The apps command line object.
 * @param app The GTK application itself.
 * @param version The version number to print.
 * @param help The help message to print.
 * @return int status code.
 */
const int init_cli(const Glib::RefPtr<Gio::ApplicationCommandLine>& cmd,
                   const Glib::RefPtr<Gtk::Application>& app,
                   const std::string version_message,
                   const std::string help_message,
                   KoMo2Model* model) {
  Glib::OptionGroup group("options", "main options");
  bool is_version = false, is_help = false,
       is_mnemonic = false;  // Booleans for options being toggled
  Glib::OptionEntry version, help, mnemonics;  // Option objects

  // Setting the version `-v` option
  version.set_long_name("version");
  version.set_short_name('v');
  version.set_description("Display version information.");
  group.add_entry(version, is_version);

  // Setting the help `-h` option
  help.set_long_name("help");
  help.set_short_name('h');
  help.set_description("Display a help message.");
  group.add_entry(help, is_help);

  mnemonics.set_long_name("english");
  mnemonics.set_short_name('e');
  mnemonics.set_description(
      "Play ARM mnemonics in English if using a screenreader.");
  group.add_entry(mnemonics, is_mnemonic);

  Glib::OptionContext context;
  context.add_group(group);

  // Parses command line arguments.
  Glib::OptionGroup gtkgroup(gtk_get_option_group(true));
  context.add_group(gtkgroup);
  int argc;
  char** argv = cmd->get_arguments(argc);
  context.parse(argc, argv);

  int return_val = handle_cli(is_version, is_help, is_mnemonic, version_message,
                              help_message, model);

  // If command line arguments were valid, activate app, else not.
  if (not return_val) {
    app->activate();
  }

  return return_val;
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
const bool receive_compiler_output(GIOChannel* source,
                                   GIOCondition condition,
                                   gpointer data) {
  // data is always a pointer to the main window
  auto p = static_cast<KoMo2Model*>(data);
  char buff[128];  // An arbitrary size

  // The amount of bytes that were read
  int x = read(compiler_comms[0], buff, 128);

  // There was a pipe reading failure.
  if (x < 0) {
    std::cout << "Error reading the compiler communication pipe." << std::endl;
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
