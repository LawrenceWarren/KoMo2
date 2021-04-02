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
#include "../views/ControlsView.h"
#include "KoMo2Model.h"

/**
 * @brief Construct a new ControlsModel::ControlsModel object.
 * @param view A pointer to the accompanying view object.
 * @param parent A pointer to the parent model.
 */
ControlsModel::ControlsModel(ControlsView* const view, KoMo2Model* const parent)
    : Model(parent), view(view) {
  // Set button listeners for each of the control buttons
  // setButtonListener(view->getHelpButton(), this,
  // &ControlsModel::onHelpClick);

  std::cout << "seg fault here?" << std::endl;

  view->getHelpButton()->set_uri(
      "https://github.com/LawrenceWarren/KoMo2#user-manual");

  std::cout << "or much after" << std::endl;

  setButtonListener(view->getPauseResumeButton(), this,
                    &ControlsModel::onPauseResumeClick);

  setButtonListener(view->getHaltExecutionButton(), this,
                    &ControlsModel::onHaltExecutionClick);

  setButtonListener(view->getReloadJimulatorButton(), this,
                    &ControlsModel::onReloadJimulatorClick);

  setButtonListener(view->getSingleStepExecuteButton(), this,
                    &ControlsModel::onSingleStepExecuteClick);

  // Set the model & images of the view.
  view->setModel(this);
  view->setButtonImages(getParent()->getAbsolutePathToProjectRoot());
}

// ! Button event handlers

/**
 * @brief Handles the `helpButton` click events.
 */
void ControlsModel::onHelpClick() {
  // TODO: Launch a seperate help window.
}

/**
 * @brief Handles the `haltExecutionButton` click events -  changes
 * `JimulatorState` to "UNLOADED".
 */
void ControlsModel::onHaltExecutionClick() {
  getParent()->changeJimulatorState(UNLOADED);
}

/**
 * @brief Handles the `reloadJimulatorButton` click events - sends a command to
 * Jimulator and changes `JimulatorState` to "LOADED".
 */
void ControlsModel::onReloadJimulatorClick() {
  Jimulator::resetJimulator();
  getParent()->changeJimulatorState(LOADED);
}

/**
 * @brief Handles the `pauseResumeButton` click events - changes
 * `JimulatorState` to "RUNNING" if currently PAUSED, and "PAUSED" if currently
 * RUNNING. Sends a command to Jimulator in every case.
 */
void ControlsModel::onPauseResumeClick() {
  switch (getJimulatorState()) {
    case RUNNING:
      Jimulator::pauseJimulator();
      getParent()->changeJimulatorState(PAUSED);
      break;
    case PAUSED:
      Jimulator::continueJimulator();
      getParent()->changeJimulatorState(RUNNING);
      break;
    case LOADED:
      Jimulator::startJimulator(0);
      getParent()->changeJimulatorState(RUNNING);
      break;
    default:
      // TODO: Handle error state gracefully
      break;
  }
}

/**
 * @brief Handles the `singleStepExecuteButton` click events - changes
 * `JimulatorState` to "PAUSED" if state is already LOADED, and sends a command
 * to Jimulator.
 */
void ControlsModel::onSingleStepExecuteClick() {
  Jimulator::startJimulator(1);
  getParent()->refreshViews();

  if (getJimulatorState() == LOADED) {
    getParent()->changeJimulatorState(PAUSED);
  }
}

// ! Virtual functions

/**
 * @brief Handles a key press event pertaining to this model.
 * @param e The key press event.
 * @return bool Was a key pressed or not?
 */
const bool ControlsModel::handleKeyPress(const GdkEventKey* const e) {
  switch (e->keyval) {
    case GDK_KEY_F5:
      if (getJimulatorState() != UNLOADED) {
        onPauseResumeClick();
      }
      return true;
    case GDK_KEY_F6:
      if (getJimulatorState() == LOADED || getJimulatorState() == PAUSED) {
        onSingleStepExecuteClick();
      }
      return true;
    case GDK_KEY_F1:
      if (getJimulatorState() == RUNNING || getJimulatorState() == PAUSED) {
        onHaltExecutionClick();
      }
      return true;
    case GDK_KEY_F12:
      onHelpClick();
      return true;
    default:
      return false;
  }

  return false;
}

/**
 * @brief Handles the Jimulator state change for this model. For each value of
 * `newState`, it will update the state of the control buttons & potentially
 * update their images.
 * @param newState The state that was changed into.
 */
void ControlsModel::changeJimulatorState(const JimulatorState newState) {
  switch (newState) {
    // some unloaded state
    case UNLOADED: {
      setButtonState(view->getHelpButton(), true);
      setButtonState(view->getReloadJimulatorButton(), false);
      setButtonState(
          view->getPauseResumeButton(), false,
          new Gtk::Image(getParent()->getAbsolutePathToProjectRoot() +
                         "res/img/commenceSymbol.png"),
          "Commence execution (F5)");
      setButtonState(view->getHaltExecutionButton(), false);
      setButtonState(view->getSingleStepExecuteButton(), false);

      // Set accessibility options for the pause/resume button
      auto pauseResume = view->getPauseResumeButton()->get_accessible();
      pauseResume->set_name("Play/pause execution");
      pauseResume->set_description("Plays and pauses execution of program");
      return;
    }
    // loaded, not yet run state
    case LOADED: {
      setButtonState(view->getHelpButton(), true);
      setButtonState(view->getReloadJimulatorButton(), false);
      setButtonState(
          view->getPauseResumeButton(), true,
          new Gtk::Image(getParent()->getAbsolutePathToProjectRoot() +
                         "res/img/commenceSymbol.png"),
          "Commence execution (F5)");
      setButtonState(view->getSingleStepExecuteButton(), true);
      setButtonState(view->getHaltExecutionButton(), false);

      // Set accessibility options for the pause/resume button
      auto pauseResume = view->getPauseResumeButton();
      pauseResume->set_tooltip_text("Commence execution (F5)");
      pauseResume->get_accessible()->set_name("Commence execution");
      pauseResume->get_accessible()->set_description(
          "Commence execution of the loaded program");
      return;
    }
    // Currently running
    case RUNNING: {
      setButtonState(view->getHelpButton(), true);
      setButtonState(view->getReloadJimulatorButton(), false);
      setButtonState(
          view->getPauseResumeButton(), true,
          new Gtk::Image(getParent()->getAbsolutePathToProjectRoot() +
                         "res/img/pauseSymbol.png"),
          "Pause execution (F5)");
      setButtonState(view->getSingleStepExecuteButton(), false);
      setButtonState(view->getHaltExecutionButton(), true);

      // Set accessibility options for the pause/resume button
      auto pauseResume = view->getPauseResumeButton();
      pauseResume->set_tooltip_text("Pause execution (F5)");
      pauseResume->get_accessible()->set_name("Pause execution");
      pauseResume->get_accessible()->set_description(
          "Pause execution of the running program");
      return;
    }
    // Has been running; is paused
    case PAUSED: {
      setButtonState(view->getHelpButton(), true);
      setButtonState(view->getReloadJimulatorButton(), true);
      setButtonState(
          view->getPauseResumeButton(), true,
          new Gtk::Image(getParent()->getAbsolutePathToProjectRoot() +
                         "res/img/playSymbol.png"),
          "Resume execution (F5)");
      setButtonState(view->getSingleStepExecuteButton(), true);
      setButtonState(view->getHaltExecutionButton(), true);

      // Set accessibility options for the pause/resume button
      auto pauseResume = view->getPauseResumeButton();
      pauseResume->set_tooltip_text("Resume execution (F5)");
      pauseResume->get_accessible()->set_name("Resume execution");
      pauseResume->get_accessible()->set_description(
          "Resume execution of the paused program");
      return;
    }
    // Error state
    default:
      // TODO: Handle the error state gracefully
      return;
  }
}
