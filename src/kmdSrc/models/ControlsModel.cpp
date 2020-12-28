/**
 * @file ControlsModel.cpp
 * @author Lawrence Warren (lawrencewarren2@gmail.com)
 * @brief Definitions of the functions declared in the class declaration, found
 * in the file `ControlsModel.h`.
 * @version 0.1
 * @date 2020-12-28
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

#include <iostream>
#include <string>
#include "../compile.h"
#include "../views/ControlsView.h"
#include "KoMo2Model.h"

/**
 * @brief Construct a new ControlsModel object.
 * @param helpButton A pointer to the helpButton.
 * @param reloadJimulatorButton A pointer to the reloadJimulatorButton.
 * @param pauseResumeButton A pointer to the pauseResumeButton.
 * @param singleStepExecuteButton A pointer to the singleStepExecutionButton.
 * @param haltExecutionButton A pointer to the haltExecutionButton.
 * @param parent A pointer to the parent model, KoMo2Model.
 */
ControlsModel::ControlsModel(ControlsView* view, KoMo2Model* parent)
    : Model(parent), view(view) {
  setButtonListener(view->getHelpButton(), this, &ControlsModel::onHelpClick);

  setButtonListener(view->getPauseResumeButton(), this,
                    &ControlsModel::onPauseResumeClick);

  setButtonListener(view->getHaltExecutionButton(), this,
                    &ControlsModel::onHaltExecutionClick);

  setButtonListener(view->getReloadJimulatorButton(), this,
                    &ControlsModel::onReloadJimulatorClick);

  setButtonListener(view->getSingleStepExecuteButton(), this,
                    &ControlsModel::onSingleStepExecuteClick);

  view->setModel(this);
}

/**
 * @brief Destroys a ControlModel object.
 */
ControlsModel::~ControlsModel() {}

/**
 * @brief Handles a key press event pertaining to this model.
 * @param e The key press event.
 * @return bool Was a key pressed or not?
 */
bool ControlsModel::handleKeyPress(GdkEventKey* e) {
  switch (e->keyval) {
    // F5
    case 65474:
      if (getJimulatorState() != UNLOADED) {
        onPauseResumeClick();
      }
      return true;

    // F6
    case 65475:
      if (getJimulatorState() == LOADED || getJimulatorState() == PAUSED) {
        onSingleStepExecuteClick();
      }
      return true;

    // F1
    case 65470:
      onHelpClick();
      return true;

    // F12
    case 65481:
      if (getJimulatorState() == RUNNING || getJimulatorState() == PAUSED) {
        onHaltExecutionClick();
      }
      return true;

    // Otherwise
    default:
      return false;
  }

  return false;
}

/**
 * @brief Handles the `helpButton` click events.
 */
void ControlsModel::onHelpClick() {
  std::cout << "Help Button click!" << std::endl;
}

/**
 * @brief Handles the `reloadJimulatorButton` click events.
 * Changes JimulatorState to "LOADED".
 */
void ControlsModel::onReloadJimulatorClick() {
  std::cout << "Reload clicked!" << std::endl;
  // getParent()->getCompileLoadModel()->onCompileLoadClick();
  resetJimulator();
  // TODO: Update some views
  getParent()->changeJimulatorState(LOADED);
}

/**
 * @brief Handles the `pauseResumeButton` click events.
 * Changes JimulatorState to "RUNNING" if currently PAUSED, and "PAUSED" if
 * currently RUNNING.
 */
void ControlsModel::onPauseResumeClick() {
  std::cout << "pause/Resume Button Click!" << std::endl;

  switch (getJimulatorState()) {
    case RUNNING:
      pauseJimulator();
      // TODO: update some views
      getParent()->changeJimulatorState(PAUSED);
      break;
    case PAUSED:
      continueJimulator();
      // TODO: update some views
      getParent()->changeJimulatorState(RUNNING);
      break;
    case LOADED:
      start(0);
      getParent()->changeJimulatorState(RUNNING);
      break;
    default:
      // TODO: Handle error state gracefully
      break;
  }
}

/**
 * @brief Handles the `singleStepExecuteButton` click events.
 */
void ControlsModel::onSingleStepExecuteClick() {
  std::cout << "single step execute click!" << std::endl;

  if (getJimulatorState() == LOADED) {
    getParent()->changeJimulatorState(PAUSED);
  }
}

/**
 * @brief Handles the `haltExecutionButton` click events.
 * Changes JimulatorState to "UNLOADED".
 */
void ControlsModel::onHaltExecutionClick() {
  std::cout << "Halt Execution Button Click!" << std::endl;
  getParent()->changeJimulatorState(UNLOADED);
}

/**
 * @brief Handles the Jimulator state change for this model.
 * @param newState The state to change into.
 */
void ControlsModel::changeJimulatorState(JimulatorState newState) {
  switch (newState) {
    // some unloaded state
    case UNLOADED:
      setButtonState(view->getHelpButton(), true);
      setButtonState(view->getReloadJimulatorButton(), false);
      setButtonState(view->getPauseResumeButton(), false,
                     "Commence execution (F5)",
                     new Gtk::Image("res/commenceSymbol.png"));
      setButtonState(view->getHaltExecutionButton(), false);
      setButtonState(view->getSingleStepExecuteButton(), false);
      break;

    // loaded, not yet run state
    case LOADED:
      setButtonState(view->getHelpButton(), true);
      setButtonState(view->getReloadJimulatorButton(), false);
      setButtonState(view->getPauseResumeButton(), true,
                     "Commence execution (F5)",
                     new Gtk::Image("res/commenceSymbol.png"));
      setButtonState(view->getSingleStepExecuteButton(), true);
      setButtonState(view->getHaltExecutionButton(), false);
      break;

    // Currently running
    case RUNNING:
      setButtonState(view->getHelpButton(), true);
      setButtonState(view->getReloadJimulatorButton(), false);
      setButtonState(view->getPauseResumeButton(), true, "Pause execution (F5)",
                     new Gtk::Image("res/pauseSymbol.png"));
      setButtonState(view->getSingleStepExecuteButton(), false);
      setButtonState(view->getHaltExecutionButton(), true);
      // Send some signal to Jimulator
      break;

    // Has been running; is paused
    case PAUSED:
      setButtonState(view->getHelpButton(), true);
      setButtonState(view->getReloadJimulatorButton(), true);
      setButtonState(view->getPauseResumeButton(), true,
                     "Resume execution (F5)",
                     new Gtk::Image("res/playSymbol.png"));
      setButtonState(view->getSingleStepExecuteButton(), true);
      setButtonState(view->getHaltExecutionButton(), true);
      // Send some signal to Jimulator
      break;

    // Error state
    default:
      // TODO: Handle the error state gracefully
      break;
  }
}
