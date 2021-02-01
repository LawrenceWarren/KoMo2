/**
 * @file KoMo2Model.cpp
 * @author Lawrence Warren (lawrencewarren2@gmail.com)
 * @brief Definitions of the functions declared in the class declaration, found
 * at `KoMo2Model.h`.
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

#include "KoMo2Model.h"
#include <glibmm.h>
#include <gtkmm/filechooserdialog.h>
#include <iostream>
#include "../views/MainWindowView.h"

/**
 * @brief Construct a new KoMo2Model - this constructor initialises the
 * mainWindow pointer, as well as the absolutePathToProjectRoot member. It then
 * constructs a member compileLoadModel, and sets the compile buttons on click
 * events.
 * @param mainWindow A pointer to the mainWindow view object.
 * @param argv0 The absolutePathToProjectRoot - parsed from argv[0].
 */
KoMo2Model::KoMo2Model(MainWindowView* mainWindow, std::string argv0)
    : Model(this),
      mainWindow(mainWindow),
      absolutePathToProjectRoot(argv0),
      compileLoadModel(mainWindow->getCompileLoadView(), this),
      controlsModel(mainWindow->getControlsView(), this),
      registersModel(mainWindow->getRegistersView(), this) {
  // Updates the main window to have a pointer to its model, sets its CSS.
  getMainWindow()->setModel(this);
  getMainWindow()->setStyling();

  // Sets key down events to fire on this handleKeyPress method
  getMainWindow()->signal_key_press_event().connect(
      sigc::mem_fun(*this, &Model::handleKeyPress), false);

  this->changeJimulatorState(UNLOADED);
}



/**
 * @brief Refreshes the views. May be called on a looping timer.
 * @return bool True if to be called in a loop.
 */
bool KoMo2Model::refreshViews() {
  if (getJimulatorState() == RUNNING) {
    // TODO: look at KoMoDo function callback_updateall()
    std::cout << "refresh views on this timer!" << std::endl;
    registersModel.getView()->refreshViews();
    return true;
  } else {
    // TODO: Refresh views once
    std::cout << "refreshing views once" << std::endl;
    registersModel.getView()->refreshViews();
    return false;
  }
}

/**
 * @brief Passes the key press event off to other child models.
 * @param e The key press event.
 * @return bool if a key was pressed.
 */
bool KoMo2Model::handleKeyPress(GdkEventKey* e) {
  if (getCompileLoadModel()->handleKeyPress(e)) {
    return true;
  } else if (getControlsModel()->handleKeyPress(e)) {
    return true;
  }

  return false;
}

/**
 * @brief Changes the Jimulator state and calls each child models own
 * `changeJimulatorState` function.
 * @param newState The state to change into.
 */
void KoMo2Model::changeJimulatorState(JimulatorState newState) {
  // No state change, do nothing
  if (getJimulatorState() == newState) {
    return;
  }

  // Handles refreshing the views
  switch (newState) {
    case RUNNING:
      // Calls refreshViews every 300ms
      Glib::signal_timeout().connect(
          sigc::mem_fun(this, &KoMo2Model::refreshViews), 300);
      break;
    case LOADED:
      this->refreshViews();
      break;
    case UNLOADED:
      this->refreshViews();
      break;
    default:
      break;
  }

  setJimulatorState(newState);

  // Change the state of each child model
  compileLoadModel.changeJimulatorState(newState);
  controlsModel.changeJimulatorState(newState);
  registersModel.changeJimulatorState(newState);
  // TODO: add other models here as they come
}

// ! Getter functions

/**
 * @brief Gets the `mainWindow` member variable.
 * @return MainWindowView* A pointer to the `MainWindow.`
 */
MainWindowView* KoMo2Model::getMainWindow() {
  return mainWindow;
}
/**
 * @brief Gets the `absolutePathToProjectRoot` member variable.
 * @return const std::string The absolute path to the project root.
 */
const std::string KoMo2Model::getAbsolutePathToProjectRoot() {
  return absolutePathToProjectRoot;
}
/**
 * @brief Gets the `compileLoadModel` member variable.
 * @return CompileLoadModel* A pointer to the `compileLoadModel`.
 */
CompileLoadModel* KoMo2Model::getCompileLoadModel() {
  return &compileLoadModel;
}

/**
 * @brief Gets the `controlsModel` member variable.
 * @return ControlsModel* A pointer to the `controlsModel`.
 */
ControlsModel* KoMo2Model::getControlsModel() {
  return &controlsModel;
}
