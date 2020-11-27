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
CompileLoadButton::CompileLoadButton(const char* labelText)
    : Gtk::Button(labelText) {
  signal_clicked().connect(sigc::mem_fun(*this, &CompileLoadButton::onClick));
}

/**
 * @brief Compiles a `.s` file into a `.kmd` file and then load it into
 * Jimulator, if a valid file path is given.
 */
void CompileLoadButton::onClick() {
  // Won't evaluate `fork()` if the string compare succeeds.
  if (absolutePathToSelectedFile.compare("") && !fork()) {
    compile(absolutePathToSelectedFile.c_str());
    _exit(0);
  }
  // The parent waits for the child and then.
  else {
    wait(0);
  }
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