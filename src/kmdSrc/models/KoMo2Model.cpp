/**
 * @file KoMo2Model.cpp
 * @author Lawrence Warren (lawrencewarren2@gmail.com)
 * @brief Definitions of the functions declared in the class declaration, found
 * at `KoMo2Model.h`.
 * @version 1.0.0
 * @date 10-04-2021
 */

#include "KoMo2Model.h"
#include <atkmm/action.h>
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
 * @param manual A URI that describes where the user manual can be found.
 * @param refreshRate An integer that describes how many milliseconds should be
 * taken between refreshes when KoMo2 is in the `JimulatorState::RUNNING` state.
 */
KoMo2Model::KoMo2Model(MainWindowView* const mainWindow,
                       const std::string argv0,
                       const std::string manual,
                       const int refreshRate)
    : Model(this),
      mainWindow(mainWindow),
      absolutePathToProjectRoot(argv0),
      compileLoadModel(mainWindow->getCompileLoadView(), this),
      controlsModel(mainWindow->getControlsView(), manual, this),
      registersModel(mainWindow->getRegistersView(), this),
      terminalModel(mainWindow->getTerminalView(), this),
      disassemblyModel(mainWindow->getDisassemblyView(), this),
      refreshRate(refreshRate) {
  // Updates the main window to have a pointer to its model, sets its CSS.
  getMainWindow()->setModel(this);
  getMainWindow()->setStyling();

  // Sets key down events to fire on this handleKeyPress method
  getMainWindow()->signal_key_press_event().connect(
      sigc::mem_fun(*this, &Model::handleKeyPress), false);

  changeJimulatorState(JimulatorState::UNLOADED);
}

/**
 * @brief Refreshes the views. May be called on a looping timer.
 * @return bool True if to be called in a loop, otherwise False.
 */
const bool KoMo2Model::refreshViews() {
  // Check the state of the board first
  switch (Jimulator::checkBoardState()) {
    case ClientState::FINISHED:
      getParent()->changeJimulatorState(JimulatorState::UNLOADED);
      break;
    case ClientState::BREAKPOINT:
      getParent()->changeJimulatorState(JimulatorState::PAUSED);
      break;
    default:
      break;
  }

  // Updates registers
  registersModel.refreshViews();
  disassemblyModel.refreshViews();
  terminalModel.appendTextToTextView(terminalModel.readJimulator());

  // Returns true if this function should continue looping (i.e. is running)
  return getJimulatorState() == JimulatorState::RUNNING;
}

/**
 * @brief Passes the key press event off to other child models.
 * @param e The key press event.
 * @return true if a key press was handled by the model.
 * @return false if a key press was not handled by the model by the model.
 */
const bool KoMo2Model::handleKeyPress(const GdkEventKey* const e) {
  return getTerminalModel()->handleKeyPress(e) ||
         getControlsModel()->handleKeyPress(e) ||
         getCompileLoadModel()->handleKeyPress(e) ||
         getDisassemblyModel()->handleKeyPress(e) ||
         getRegistersModel()->handleKeyPress(e);
}

/**
 * @brief Changes the Jimulator state and calls each child models own
 * `changeJimulatorState` function.
 * @param newState The state to change into.
 */
void KoMo2Model::changeJimulatorState(const JimulatorState newState) {
  // No state change, do nothing
  if (getJimulatorState() == newState) {
    return;
  }

  // Update the overall state of Jimulator State
  setJimulatorState(newState);

  // Handles refreshing the views
  switch (newState) {
    case JimulatorState::RUNNING:
      Glib::signal_timeout().connect(
          sigc::mem_fun(this, &KoMo2Model::refreshViews), refreshRate);
      break;
    case JimulatorState::LOADED:
    case JimulatorState::UNLOADED:
      refreshViews();
      break;
    default:
      break;
  }

  // Change the state of each child model
  compileLoadModel.changeJimulatorState(newState);
  controlsModel.changeJimulatorState(newState);
  registersModel.changeJimulatorState(newState);
  disassemblyModel.changeJimulatorState(newState);
  terminalModel.changeJimulatorState(newState);
}

// !!!!!!!!!!!!!!!!!!!!!!!
// ! Getters and setters !
// !!!!!!!!!!!!!!!!!!!!!!!

/**
 * @brief Gets the `mainWindow` member variable.
 * @return MainWindowView* A pointer to the `MainWindow.`
 */
MainWindowView* const KoMo2Model::getMainWindow() const {
  return mainWindow;
}
/**
 * @brief Gets the `compileLoadModel` member variable.
 * @return CompileLoadModel* A pointer to the `compileLoadModel`.
 */
CompileLoadModel* const KoMo2Model::getCompileLoadModel() {
  return &compileLoadModel;
}
/**
 * @brief Gets the `controlsModel` member variable.
 * @return ControlsModel* A pointer to the `controlsModel`.
 */
ControlsModel* const KoMo2Model::getControlsModel() {
  return &controlsModel;
}
/**
 * @brief Gets the `RegistersModel` member variable.
 * @return RegistersModel* A pointer to the `RegistersModel`.
 */
RegistersModel* const KoMo2Model::getRegistersModel() {
  return &registersModel;
}
/**
 * @brief Gets the `disassemblyModel` member variable.
 * @return DisassemblyModel* A pointer to the `disassemblyModel`.
 */
DisassemblyModel* const KoMo2Model::getDisassemblyModel() {
  return &disassemblyModel;
}
/**
 * @brief Gets the `terminalModel` member variable.
 * @return TerminalModel* A pointer to the `terminalModel`.
 */
TerminalModel* const KoMo2Model::getTerminalModel() {
  return &terminalModel;
}
/**
 * @brief Gets the `absolutePathToProjectRoot` member variable.
 * @return const std::string The absolute path to the project root.
 */
const std::string KoMo2Model::getAbsolutePathToProjectRoot() const {
  return absolutePathToProjectRoot;
}
