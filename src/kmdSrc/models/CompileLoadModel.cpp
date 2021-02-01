/**
 * @file compileLoadController.cpp
 * @author Lawrence Warren (lawrencewarren2@gmail.com)
 * @brief Definitions of the functions declared in the class declaration, found
 * at `CompileLoadModel.h`.
 * @version 0.1
 * @date 2020-12-22
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

#include <gtkmm/filechooserdialog.h>
#include <sys/wait.h>
#include <unistd.h>
#include <iostream>
#include <regex>
#include <string>
#include "../jimulatorInterface.h"
#include "../views/MainWindowView.h"
#include "KoMo2Model.h"

/**
 * @brief Construct a new CompileLoadModel, initialising the parent pointer and
 * both button pointers.
 * @param compileLoadButton a pointer to the compileLoadButton.
 * @param browseButton a pointer to the browseButton.
 * @param parent a pointer to the parent KoMo2Model.
 */
CompileLoadModel::CompileLoadModel(CompileLoadView* view, KoMo2Model* parent)
    : Model(parent), view(view) {
  // Set the onClick events for the browse and compile and load buttons to
  // be wired to CompileLoadModel member functions.
  setButtonListener(view->getBrowseButton(), this,
                    &CompileLoadModel::onBrowseClick);

  setButtonListener(view->getCompileAndLoadButton(), this,
                    &CompileLoadModel::onCompileLoadClick);

  view->setModel(this);
  changeInnerState(NO_FILE);
}

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
  if (not getAbsolutePathToSelectedFile().length()) {
    std::cout << "No file selected!" << std::endl;
    return;
  }

  // child process
  if (not fork()) {
    // Compile the .s program to .kmd
    compileJimulator(
        (getParent()->getAbsolutePathToProjectRoot() + "/bin/aasm").c_str(),
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

    // Perform the load
    resetJimulator();
    status =
        loadJimulator(makeKmdPath(getAbsolutePathToSelectedFile()).c_str());

    // If load function failed
    if (status) {
      std::cout << "Error loading file into KoMo2" << std::endl;
      return;
    }

    std::cout << "File loaded!" << std::endl;

    // ! Update the overall program state
    getParent()->changeJimulatorState(LOADED);
  }
}

/**
 * @brief Opens a file selection dialog upon the `BrowseButtonView` being
 * clicked.
 */
void CompileLoadModel::onBrowseClick() {
  // Creates a new file browser dialogue box.
  Gtk::FileChooserDialog dialog(" File explorer",
                                Gtk::FILE_CHOOSER_ACTION_OPEN);

  // Add class for styling
  // dialog.get_style_context()->add_class("dialog");
  // TODO: Add styling to the dialog box

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
    // A file was selected - update inner state and overall state
    case (Gtk::RESPONSE_OK): {
      setAbsolutePathToSelectedFile(dialog->get_filename());
      changeInnerState(FILE_SELECTED);
      getParent()->changeJimulatorState(UNLOADED);
      break;
    }
    // Dialog was cancelled - update inner state but not overall state
    case (Gtk::RESPONSE_CANCEL): {
      setAbsolutePathToSelectedFile("");
      changeInnerState(NO_FILE);
      break;
    }
    default: {
      // Some unexpected behaviour - update inner state but not overall state
      setAbsolutePathToSelectedFile("");
      changeInnerState(NO_FILE);
      break;
    }
  }
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

/**
 * @brief Handles a changing JimulatorState for this model.
 * @param newState The state that has been changed into.
 */
void CompileLoadModel::changeJimulatorState(JimulatorState newState) {
  // Sets the default button state for compileLoadButton
  if (getInnerState() == NO_FILE) {
    setButtonState(view->getCompileAndLoadButton(), false);
  } else {
    setButtonState(view->getCompileAndLoadButton(), true);
  }

  // Sets the state of the browseButton
  switch (newState) {
    // some unloaded state
    case UNLOADED:
      setButtonState(view->getBrowseButton(), true);
      break;

    // loaded, not yet run state
    case LOADED:
      setButtonState(view->getBrowseButton(), true);
      setButtonState(view->getCompileAndLoadButton(), false);
      break;

    // Currently running
    case RUNNING:
      setButtonState(view->getBrowseButton(), false);
      setButtonState(view->getCompileAndLoadButton(), false);
      break;

    // Has been running; is paused
    case PAUSED:
      setButtonState(view->getBrowseButton(), true);
      break;

    // Error state
    default:
      // TODO: Handle some error state gracefully
      break;
  }
}

/**
 * @brief Handles a key press event for this model.
 * @param e The key press event in question.
 * @return bool was the key pressed or not?
 */
bool CompileLoadModel::handleKeyPress(GdkEventKey* e) {
  switch (e->keyval) {
    // Ctrl + (lower case l)
    case 108:
      if (e->state == 4 && getJimulatorState() != RUNNING) {
        onBrowseClick();
      }
      return true;
    // ctrl + (upper case L)
    case 76:
      if (e->state == 6 && getJimulatorState() != RUNNING) {
        onBrowseClick();
      }
      return true;
    // Ctrl + (lower case r)
    case 114:
      if (e->state == 4 &&
          (getJimulatorState() != RUNNING && getInnerState() != NO_FILE)) {
        onCompileLoadClick();
      }
      return true;
    // Ctrl + (upper case R)
    case 82:
      if (e->state == 6 &&
          (getJimulatorState() != RUNNING && getInnerState() != NO_FILE)) {
        onCompileLoadClick();
      }
      return true;
      // NOTHING
    default:
      return false;
  }

  return false;
}

/**
 * @brief Handles changing the inner state of this model (whether a file is
 * selected or not)
 * @param val The value to set the inner state to.
 */
void CompileLoadModel::changeInnerState(CompileLoadInnerState val) {
  // This regex matches any length of characters up to a `/` character. The
  // regex_replace replaces them with "". So if we have `/user/demo/someFile.s`
  // it will resolve to simply become `someFile.s`
  std::string filename =
      regex_replace(getAbsolutePathToSelectedFile(), std::regex("(.*\\/)"), "");

  // Sets the label text to the filename
  view->setSelectedFileLabelText("File: " + filename);

  innerState = val;

  switch (val) {
    case FILE_SELECTED:
      setButtonState(view->getCompileAndLoadButton(), true);
      getParent()->getMainWindow()->set_title(" KoMo2 - " + filename);
      break;
    case NO_FILE:
      setButtonState(view->getCompileAndLoadButton(), false);
      getParent()->getMainWindow()->set_title(" KoMo2");
      break;
    default:
      // Error state
      break;
  }
}

// ! Getters and setters!

/**
 * @brief Sets the `absolutePathToSelectedFile` member variable.
 * @param val The value to set the `absolutePathToSelectedFile` member to.
 */
void CompileLoadModel::setAbsolutePathToSelectedFile(std::string val) {
  absolutePathToSelectedFile = val;
}
/**
 * @brief Gets the `absolutePathToSelectedFile` member variable.
 * @return std::string The `absolutePathToSelectedFile` member variable.
 */
std::string CompileLoadModel::getAbsolutePathToSelectedFile() {
  return absolutePathToSelectedFile;
}
/**
 * @brief Get the Inner State object
 * @return CompileLoadInnerState
 */
CompileLoadInnerState CompileLoadModel::getInnerState() {
  return innerState;
}
