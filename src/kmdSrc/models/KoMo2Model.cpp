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
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details at
 * https://www.gnu.org/copyleft/gpl.html
 * @copyright Copyright (c) 2020
 */

#include "KoMo2Model.h"
#include "../views/MainWindow.h"

/**
 * @brief Construct a new KoMo2Model - this constructor initialises the
 * mainWindow pointer, as well as the absolutePathToProjectRoot member. It then
 * constructs a member compileLoadModel, and sets the compile buttons on click
 * events.
 * @param mainWindow A pointer to the mainWindow view object.
 * @param argv0 The absolutePathToProjectRoot - parsed from argv[0].
 */
KoMo2Model::KoMo2Model(MainWindow* mainWindow, std::string argv0)
    : Model(this),
      mainWindow(mainWindow),
      absolutePathToProjectRoot(argv0),
      compileLoadModel(mainWindow->getCompileAndLoadButton(),
                       mainWindow->getBrowseButton(),
                       this),
      controlsModel(mainWindow->getHelpButton(),
                    mainWindow->getReloadJimulatorButton(),
                    mainWindow->getPauseResumeButton(),
                    mainWindow->getSingleStepExecuteButton(),
                    mainWindow->getHaltExecutionButton(),
                    this) {
  // Updates the main window to have a pointer to its model, sets its CSS.
  getMainWindow()->setModel(this);
  getMainWindow()->setStyling();

  // TODO: set some key-presses linked to buttons

  // Set the onClick events for the browse and compile and load buttons to
  // be wired to CompileLoadModel member functions.
  setButtonListener(getMainWindow()->getBrowseButton(), getCompileLoadModel(),
                    &CompileLoadModel::onBrowseClick);

  setButtonListener(getMainWindow()->getCompileAndLoadButton(),
                    getCompileLoadModel(),
                    &CompileLoadModel::onCompileLoadClick);

  setButtonListener(getMainWindow()->getHelpButton(), getControlsModel(),
                    &ControlsModel::onHelpClick);

  setButtonListener(getMainWindow()->getPauseResumeButton(), getControlsModel(),
                    &ControlsModel::onPauseResumeClick);

  setButtonListener(getMainWindow()->getHaltExecutionButton(),
                    getControlsModel(), &ControlsModel::onHaltExecutionClick);

  setButtonListener(getMainWindow()->getReloadJimulatorButton(),
                    getControlsModel(), &ControlsModel::onReloadJimulatorClick);

  setButtonListener(getMainWindow()->getSingleStepExecuteButton(),
                    getControlsModel(),
                    &ControlsModel::onSingleStepExecuteClick);

  this->changeJimulatorState(UNLOADED);
}

/**
 * @brief Destroys a KoMo2Model.
 */
KoMo2Model::~KoMo2Model() {}

/**
 * @brief Changes an existing image, `toChange,` to refer to a new image, and
 * deletes the old memory in the heap.
 * @param toChange A pointer to the image that is to change.
 * @param newImg The new image to change to.
 */
void KoMo2Model::changeImage(Gtk::Image* toChange, Gtk::Image* newImg) {
  Gtk::Image* temp = toChange;
  toChange = newImg;
  delete (temp);
}

/**
 * @brief Connect any button to any member function of
 * @tparam T1 A pointer type to some object of any type.
 * @tparam T2 A pointer to a T1 member function.
 * @param button A pointer to the button to set the `onClick` event for.
 * @param b A pointer to some object.
 * @param c A pointer to some member function of the b object.
 */
template <class T1, class T2>
void KoMo2Model::setButtonListener(Gtk::Button* button, T1 b, T2 c) {
  button->signal_clicked().connect(sigc::mem_fun(*b, c));
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

  setJimulatorState(newState);
  compileLoadModel.changeJimulatorState(newState);
  controlsModel.changeJimulatorState(newState);
  // TODO: add other models here as they come
}

// ! Getter functions

/**
 * @brief Gets the `mainWindow` member variable.
 * @return MainWindow* A pointer to the `MainWindow.`
 */
MainWindow* KoMo2Model::getMainWindow() {
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
