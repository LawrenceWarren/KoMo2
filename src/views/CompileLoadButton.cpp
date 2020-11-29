/**
 * @file CompileLoadButton.cpp
 * @author Lawrence Warren (lawrencewarren2@gmail.com)
 * @brief The definition of the class `CompileLoadButton` and all of it's
 * members and functions. This view handles an onClick event of compiling and
 * loading a `.s` file into the Jimulator emulator.
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

#include "CompileLoadButton.h"
#include <stdio.h>
#include <sys/wait.h>
#include <unistd.h>
#include <iostream>
#include <string>
#include "../compile.h"

/**
 * @brief Construct a new `CompileLoadButton` object which calls it's
 * superconstructor and set's the `onClick` event.
 * @param labelText The text to display on the button.
 */
CompileLoadButton::CompileLoadButton(const char* labelText,
                                     std::string absolutePathCalledFrom)
    : Gtk::Button(labelText) {
  signal_clicked().connect(sigc::mem_fun(*this, &CompileLoadButton::onClick));
  this->absolutePathCalledFrom = absolutePathCalledFrom;
}

/**
 * @brief Compiles a `.s` file into a `.kmd` file and then load it into
 * Jimulator, if a valid file path is given.
 */
void CompileLoadButton::onClick() {
  // Won't evaluate `fork()` if the string compare succeeds.
  if (absolutePathToSelectedFile.compare("") && !fork()) {
    compile((absolutePathCalledFrom + "/aasm").c_str(),
            absolutePathToSelectedFile.c_str(),
            makeKmdPath(absolutePathToSelectedFile).c_str());
    _exit(0);
  }
  // The parent waits for the child and then loads.
  else {
    wait(0);
    load(makeKmdPath(absolutePathToSelectedFile).c_str());
    std::cout << "File loaded!" << std::endl;
  }
}

/**
 * @brief Takes an ARM assembly file, removes it's current `s` extension, and
 * appends `kmd`. For example, `/home/user/demo.s` will return
 * `home/user/demo.kmd`.
 * @param absolutePath The absolute path to the `.s` program.
 * @return std::string The absolute path with just the file name.
 */
std::string CompileLoadButton::makeKmdPath(std::string absolutePath) {
  return absolutePath.substr(0, absolutePath.size() - 1).append("kmd");
}

/**
 * @brief sets the `absolutePathToSelectedFile` string.
 * @param val The value to set the string to.
 */
void CompileLoadButton::setAbsolutePathToSelectedFile(std::string val) {
  absolutePathToSelectedFile = val;
}

/**
 * @brief Gets the `absolutePathToSelectedFile` string.
 * @return std::string `absolutePathToSelectedFile`
 */
std::string CompileLoadButton::getAbsolutePathToSelectedFile() {
  return absolutePathToSelectedFile;
}