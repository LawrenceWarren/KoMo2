/**
 * @file ControlsModel.cpp
 * @author Lawrence Warren (lawrencewarren2@gmail.com)
 * @brief Definitions of the functions declared in the class declaration, found
 * in the file `ControlsModel.h`.
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

#include <gtkmm.h>
#include <gtkmm/image.h>
#include <iostream>
#include <string>
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
ControlsModel::ControlsModel(Gtk::Button* helpButton,
                             Gtk::Button* reloadJimulatorButton,
                             Gtk::Button* pauseResumeButton,
                             Gtk::Button* singleStepExecuteButton,
                             Gtk::Button* haltExecutionButton,
                             KoMo2Model* parent)
    : Model(parent),
      helpButton(helpButton),
      reloadJimulatorButton(reloadJimulatorButton),
      pauseResumeButton(pauseResumeButton),
      singleStepExecuteButton(singleStepExecuteButton),
      haltExecutionButton(haltExecutionButton) {}

/**
 * @brief Destroys a ControlModel object.
 */
ControlsModel::~ControlsModel() {}

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
  std::cout << "Reload Jimulator Button Click!" << std::endl;
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
      getParent()->changeJimulatorState(PAUSED);
      break;
    case PAUSED:
      getParent()->changeJimulatorState(RUNNING);
      break;
    case LOADED:
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
      setButtonState(helpButton, true);
      setButtonState(reloadJimulatorButton, false);
      setButtonState(pauseResumeButton, false, "Commence execution (F5)",
                     new Gtk::Image("res/commenceSymbol.png"));
      setButtonState(singleStepExecuteButton, false);
      setButtonState(haltExecutionButton, false);
      break;

    // loaded, not yet run state
    case LOADED:
      setButtonState(helpButton, true);
      setButtonState(reloadJimulatorButton, false);
      setButtonState(pauseResumeButton, true, "Commence execution (F5)",
                     new Gtk::Image("res/commenceSymbol.png"));
      setButtonState(singleStepExecuteButton, true);
      setButtonState(haltExecutionButton, false);
      break;

    // Currently running
    case RUNNING:
      setButtonState(helpButton, true);
      setButtonState(reloadJimulatorButton, false);
      setButtonState(pauseResumeButton, true, "Pause execution (F5)",
                     new Gtk::Image("res/pauseSymbol.png"));
      setButtonState(singleStepExecuteButton, false);
      setButtonState(haltExecutionButton, true);
      // Send some signal to Jimulator
      break;

    // Has been running; is paused
    case PAUSED:
      setButtonState(helpButton, true);
      setButtonState(reloadJimulatorButton, true);
      setButtonState(pauseResumeButton, true, "Resume execution (F5)",
                     new Gtk::Image("res/playSymbol.png"));
      setButtonState(singleStepExecuteButton, true);
      setButtonState(haltExecutionButton, true);
      // Send some signal to Jimulator
      break;

    // Error state
    default:
      // TODO: Handle the error state gracefully
      break;
  }
}
