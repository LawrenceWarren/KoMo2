/**
 * @file compileLoadController.cpp
 * @author your name (you@domain.com)
 * @brief
 * @version 0.1
 * @date 2020-12-21
 *
 * @copyright Copyright (c) 2020
 *
 */

// #include "CompileLoadModel.h"
#include <gtkmm.h>
#include <gtkmm/filechooserdialog.h>
#include <stdio.h>
#include <sys/wait.h>
#include <unistd.h>
#include <iostream>
#include <regex>
#include <string>
#include "../compile.h"
#include "../views/MainWindow.h"
#include "KoMo2Model.h"

/**
 * @brief Construct a new CompileLoadModel, initialising the parent pointer and
 * both button pointers.
 * @param compileLoadButton a pointer to the compileLoadButton.
 * @param browseButton a pointer to the browseButton.
 * @param parent a pointer to the parent KoMo2Model.
 */
CompileLoadModel::CompileLoadModel(Gtk::Button* compileLoadButton,
                                   Gtk::Button* browseButton,
                                   KoMo2Model* parent)
    : parent(parent),
      compileLoadButton(compileLoadButton),
      browseButton(browseButton) {}

/**
 * @brief Destroys a CompileLoadModel.
 */
CompileLoadModel::~CompileLoadModel() {}

/**
 * @brief Compiles a `.s` file into a `.kmd` file:
 * Forks a child process, executes aasm on the child, and then load it into
 * Jimulator, if a valid file path is given.
 */
void CompileLoadModel::onCompileLoadClick() {
  // If the length is zero, invalid path
  if (!getAbsolutePathToSelectedFile().length()) {
    std::cout << "No file selected!" << std::endl;
    return;
  }

  // child process
  if (!fork()) {
    // Compile the .s program to .kmd
    compile((getParent()->getAbsolutePathToProjectRoot() + "/bin/aasm").c_str(),
            getAbsolutePathToSelectedFile().c_str(),
            makeKmdPath(getAbsolutePathToSelectedFile()).c_str());
    _exit(0);
  }

  // parent process
  else {
    int status = 0;
    wait(&status);  // Wait for child to return

    // If child process failed
    if (status) {
      std::cout << "aasm failed - invalid file path!" << std::endl;
      return;
    }

    // If load function failed
    else if (load(makeKmdPath(getAbsolutePathToSelectedFile()).c_str())) {
      std::cout << "Error loading file into KoMo2" << std::endl;
      return;
    }

    std::cout << "File loaded!" << std::endl;
  }
}

/**
 * @brief Opens a file selection dialog upon the `BrowseButtonView` being
 * clicked.
 */
void CompileLoadModel::onBrowseClick() {
  // Creates a new file browser dialogue box.
  Gtk::FileChooserDialog dialog("Please choose a file",
                                Gtk::FILE_CHOOSER_ACTION_OPEN);

  // Gets the parent of the dialogue box.
  dialog.set_transient_for(*getParent()->getMainWindow());

  // Add response buttons the the dialog:
  dialog.add_button("_Cancel", Gtk::RESPONSE_CANCEL);
  dialog.add_button("_Open", Gtk::RESPONSE_OK);

  // Creates a filter for what type of files can be selected.
  auto assemblyFilter = Gtk::FileFilter::create();
  assemblyFilter->set_name("ARM assembly files");
  assemblyFilter->add_pattern("*.s");
  dialog.add_filter(assemblyFilter);

  // Show the dialog and wait for a user response, then handle the result.
  handleResult(dialog.run(), &dialog);
}

/**
 * @brief Handles the result of the file browser dialog box being closed.
 * @param result The result of the file browser closing (i.e. was a file
 * selected, was the dialog box cancelled, did something unexpected happen,
 * etc.)
 * @param dialog A pointer to the dialog box itself - frees itself in its
 * destructor.
 */
void CompileLoadModel::handleResult(int result,
                                    Gtk::FileChooserDialog* dialog) {
  switch (result) {
    case (Gtk::RESPONSE_OK): {
      setAbsolutePathToSelectedFile(dialog->get_filename());
      break;
    }
    case (Gtk::RESPONSE_CANCEL): {
      setAbsolutePathToSelectedFile("");
      break;
    }
    default: {
      // Some unexpected behaviour
      break;
    }
  }

  // Sets the label text, regexing out everything but the filename
  // (specifically, this regex matches any length of characters up to a `/`
  // character. The regex_replace replaces them with "". So if we have
  // `/user/demo/someFile.s` it will resolve to simply become `someFile.s`)
  getParent()->getMainWindow()->setSelectedFileLabel(
      regex_replace(getAbsolutePathToSelectedFile(), std::regex("(.*\/)"), ""));
}

/**
 * @brief Takes an ARM assembly file, removes it's current `s` extension, and
 * appends `kmd`. For example, `/home/user/demo.s` will return
 * `home/user/demo.kmd`.
 * @param absolutePath The absolute path to the `.s` program.
 * @return std::string The absolute path with just the file name.
 */
std::string CompileLoadModel::makeKmdPath(std::string absolutePath) {
  return absolutePath.substr(0, absolutePath.size() - 1).append("kmd");
}

// ! Getters and setters!

// Absolute path to selected file
void CompileLoadModel::setAbsolutePathToSelectedFile(std::string val) {
  absolutePathToSelectedFile = val;
}
std::string CompileLoadModel::getAbsolutePathToSelectedFile() {
  return absolutePathToSelectedFile;
}

// parent pointer
KoMo2Model* CompileLoadModel::getParent() {
  return parent;
}
