/**
 * @file CompileLoadModel.cpp
 * @author Lawrence Warren (lawrencewarren2@gmail.com)
 * @brief Definitions of the functions declared in the class declaration, found
 * at `CompileLoadModel.h`.
 * @version 1.0.0
 * @date 10-04-2021
 */

#include <gtkmm/filechooserdialog.h>
#include <sys/wait.h>
#include <unistd.h>
#include <iostream>
#include <regex>
#include <string>
#include "../views/MainWindowView.h"
#include "KoMo2Model.h"

/**
 * @brief Construct a new CompileLoadModel::CompileLoadModel object.
 * @param view A pointer to the accompanying view object.
 * @param parent A pointer to the parent model object.
 */
CompileLoadModel::CompileLoadModel(CompileLoadView* const view,
                                   KoMo2Model* const parent)
    : Model(parent), view(view) {
  // Set the onClick events for the browse and compile and load buttons to
  // be wired to CompileLoadModel member functions.
  setButtonListener(view->getBrowseButton(), this,
                    &CompileLoadModel::onBrowseClick);

  setButtonListener(view->getCompileAndLoadButton(), this,
                    &CompileLoadModel::onCompileLoadClick);

  // Set's the views model & updates the compile load models inner state.
  view->setModel(this);
  setInnerState(CompileLoadInnerState::NO_FILE);
}

/**
 * @brief Compiles a `.s` file into a `.kmd` file:
 * Forks a child process, executes aasm on the child, and then load it into
 * Jimulator, if a valid file path is given.
 */
void CompileLoadModel::onCompileLoadClick() const {
  // If the length is zero, invalid path
  if (not getAbsolutePathToSelectedFile().length()) {
    // should not be reachable
    return;
  }

  // The code within this if block is executed by the child process.
  if (not fork()) {
    // Compile the .s program to .kmd
    Jimulator::compileJimulator(
        (getParent()->getAbsolutePathToProjectRoot() + "/bin/aasm").c_str(),
        getAbsolutePathToSelectedFile().c_str(),
        makeKmdPath(getAbsolutePathToSelectedFile()).c_str());
    _exit(0);
  }

  // parent process
  else {
    int status = 0;
    wait(&status);  // Wait for the child to return

    // If child process failed
    if (status) {
      return;
    }

    // Perform the load
    Jimulator::resetJimulator();

    // If load function failed
    if (not Jimulator::loadJimulator(
            makeKmdPath(getAbsolutePathToSelectedFile()).c_str())) {
      std::cout << "Error loading file into KoMo2." << std::endl;
      return;
    }

    getParent()->changeJimulatorState(JimulatorState::LOADED);
  }
}

/**
 * @brief Opens a file selection dialog upon the `BrowseButtonView` being
 * clicked.
 */
void CompileLoadModel::onBrowseClick() {
  // Creates a new file browser dialogue box
  Gtk::FileChooserDialog dialog("File explorer", Gtk::FILE_CHOOSER_ACTION_OPEN);

  // Gets the parent of the dialogue box
  dialog.set_transient_for(*getParent()->getMainWindow());

  // Add response buttons the the dialog:
  dialog.add_button("_Cancel", Gtk::RESPONSE_CANCEL);
  dialog.add_button("_Open", Gtk::RESPONSE_OK);

  // Creates a filter for what type of files can be selected
  auto assemblyFilter = Gtk::FileFilter::create();
  assemblyFilter->set_name("ARM assembly files");
  assemblyFilter->add_pattern("*.s");
  dialog.add_filter(assemblyFilter);

  // Show the dialog and wait for a user response, then handle the result
  handleResultFromFileBrowser(dialog.run(), &dialog);
}

/**
 * @brief Handles the result of the file browser dialog box being closed.
 * @param result The result of the file browser closing (i.e. was a file
 * selected, was the dialog box cancelled, did something unexpected happen,
 * etc.)
 * @param dialog A pointer to the dialog box itself - frees itself in its
 * destructor.
 */
void CompileLoadModel::handleResultFromFileBrowser(
    const int result,
    const Gtk::FileChooserDialog* const dialog) {
  switch (result) {
    // A file was selected - update inner state and overall state
    case (Gtk::RESPONSE_OK): {
      setAbsolutePathToSelectedFile(dialog->get_filename());
      setInnerState(CompileLoadInnerState::FILE_SELECTED);
      getParent()->changeJimulatorState(JimulatorState::UNLOADED);
      return;
    }
    // Dialog was cancelled - update inner state but not overall state
    case (Gtk::RESPONSE_CANCEL): {
      setAbsolutePathToSelectedFile("");
      setInnerState(CompileLoadInnerState::NO_FILE);
      return;
    }
    default: {
      // Some unexpected behaviour - update inner state but not overall state
      setAbsolutePathToSelectedFile("");
      setInnerState(CompileLoadInnerState::NO_FILE);
      return;
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
const std::string CompileLoadModel::makeKmdPath(
    const std::string absolutePath) const {
  return absolutePath.substr(0, absolutePath.size() - 1).append("kmd");
}

/**
 * @brief Handles a change in JimulatorState for this model.
 * @param newState The state that has been changed into.
 */
void CompileLoadModel::changeJimulatorState(const JimulatorState newState) {
  // Sets the default button state for compileLoadButton
  if (getInnerState() == CompileLoadInnerState::NO_FILE) {
    setButtonState(view->getCompileAndLoadButton(), false);
  } else {
    setButtonState(view->getCompileAndLoadButton(), true);
  }

  // Sets the state of the browseButton
  switch (newState) {
    // some unloaded state
    case JimulatorState::UNLOADED:
      setButtonState(view->getBrowseButton(), true);
      break;

    // loaded, not yet run state
    case JimulatorState::LOADED:
      setButtonState(view->getBrowseButton(), true);
      setButtonState(view->getCompileAndLoadButton(), false);
      break;

    // Currently running
    case JimulatorState::RUNNING:
      setButtonState(view->getBrowseButton(), false);
      setButtonState(view->getCompileAndLoadButton(), false);
      break;

    // Has been running; is paused
    case JimulatorState::PAUSED:
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
const bool CompileLoadModel::handleKeyPress(const GdkEventKey* const e) {
  // If ctrl is not pressed, return false
  if ((e->state & (GDK_SHIFT_MASK | GDK_CONTROL_MASK | GDK_MOD1_MASK)) !=
      GDK_CONTROL_MASK) {
    return false;
  }

  switch (e->keyval) {
    // Ctrl + (lower- & upper-case l)
    case GDK_KEY_L:
    case GDK_KEY_l:
      if (getJimulatorState() != JimulatorState::RUNNING) {
        onBrowseClick();
      }
      return true;
    // Ctrl + (lower- & upper-case r)
    case GDK_KEY_R:
    case GDK_KEY_r:
      if (getJimulatorState() != JimulatorState::RUNNING &&
          getInnerState() != CompileLoadInnerState::NO_FILE) {
        onCompileLoadClick();
      }
      return true;
    default:
      return false;
  }

  return false;
}

// ! Getters and setters!

/**
 * @brief Handles changing the inner state of this model (whether a file is
 * selected or not)
 * @param val The value to set the inner state to.
 */
void CompileLoadModel::setInnerState(const CompileLoadInnerState val) {
  // This regex matches any length of characters up to a `/` character. The
  // regex_replace replaces them with "". So if we have `/user/demo/someFile.s`
  // it will resolve to simply become `someFile.s`
  std::string filename =
      regex_replace(getAbsolutePathToSelectedFile(), std::regex("(.*\\/)"), "");

  // Sets the label text to the filename
  view->setSelectedFileLabelText("File: " + filename);

  innerState = val;

  switch (val) {
    case CompileLoadInnerState::FILE_SELECTED:
      setButtonState(view->getCompileAndLoadButton(), true);
      getParent()->getMainWindow()->set_title(" KoMo2 - " + filename);
      break;
    case CompileLoadInnerState::NO_FILE:
      setButtonState(view->getCompileAndLoadButton(), false);
      getParent()->getMainWindow()->set_title(" KoMo2");
      break;
    default:
      // Error state
      break;
  }
}
/**
 * @brief Sets the `absolutePathToSelectedFile` member variable.
 * @param val The value to set the `absolutePathToSelectedFile` member to.
 */
void CompileLoadModel::setAbsolutePathToSelectedFile(const std::string val) {
  absolutePathToSelectedFile = val;
}
/**
 * @brief Gets the `absolutePathToSelectedFile` member variable.
 * @return std::string The `absolutePathToSelectedFile` member variable.
 */
const std::string CompileLoadModel::getAbsolutePathToSelectedFile() const {
  return absolutePathToSelectedFile;
}
/**
 * @brief Get the Inner State object
 * @return CompileLoadInnerState
 */
const CompileLoadInnerState CompileLoadModel::getInnerState() const {
  return innerState;
}
